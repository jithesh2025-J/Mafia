#include "AIClient.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

namespace {
    std::string sanitizeForJSON(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else if (c == '\\') result += "\\\\";
            else result += c;
        }
        return result;
    }

    std::string fallbackDialogue(const std::string& role, const std::string& context)
    {
        static const char* kLines[] = {
            "I am still thinking this through.",
            "That does not add up. We should watch carefully.",
            "Someone here is lying, and I do not like it.",
            "Let us focus on facts before we vote.",
            "I have a bad feeling about this round.",
            "I am not convinced yet. Keep talking.",
        };

        size_t seed = std::hash<std::string>{}(role + "|" + context);
        return kLines[seed % (sizeof(kLines) / sizeof(kLines[0]))];
    }
}

std::string AIClient::loadApiKey() {
    std::ifstream file("api_key.txt");
    if (file.is_open()) {
        std::string key;
        std::getline(file, key);
        if (!key.empty() && key.back() == '\r') key.pop_back();
        return key;
    }
    return "";
}

std::string AIClient::parseJsonContent(const std::string& json) {
    std::string search = "\"content\": \"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.length();
    
    std::string result;
    bool escaped = false;
    for (; pos < json.length(); ++pos) {
        char c = json[pos];
        if (escaped) {
            if (c == 'n') result += '\n';
            else if (c == 'r') result += '\r';
            else if (c == 't') result += '\t';
            else result += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            break;
        } else {
            result += c;
        }
    }
    return result;
}

std::string AIClient::generateDialogue(std::string role, std::string context) {
    std::string apiKey = loadApiKey();
    if (apiKey.empty()) return fallbackDialogue(role, context);

    std::string systemPrompt = "You are an AI player in a Mafia game. Your role/personality is: " + role + ". Provide a VERY short single line of dialogue (1-2 sentences max) reacting to the context. Stay in character.";
    std::string userPrompt = "Context: " + context + "\nSay something:";

    std::string payload = "{\"model\": \"gpt-4o-mini\", \"messages\": [{\"role\": \"system\", \"content\": \"" + sanitizeForJSON(systemPrompt) + "\"}, {\"role\": \"user\", \"content\": \"" + sanitizeForJSON(userPrompt) + "\"}], \"max_tokens\": 50}";
    
    // Write payload to temp file to avoid cmd quoting issues
    std::string tempFile = "ai_payload_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".json";
    std::ofstream out(tempFile);
    out << payload;
    out.close();

    std::string cmd = "curl.exe -s https://api.openai.com/v1/chat/completions "
                      "-H \"Content-Type: application/json\" "
                      "-H \"Authorization: Bearer " + apiKey + "\" "
                      "--connect-timeout 3 -m 8 "
                      "-d @" + tempFile;

    FILE* pipe = POPEN(cmd.c_str(), "r");
    if (!pipe) {
        std::remove(tempFile.c_str());
        return fallbackDialogue(role, context);
    }

    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    PCLOSE(pipe);
    std::remove(tempFile.c_str());

    if (result.find("\"error\"") != std::string::npos)
        return fallbackDialogue(role, context);

    std::string content = parseJsonContent(result);
    // If extraction fails, use an offline-safe fallback line.
    if (content.empty()) {
        std::cout << "[AI Error Raw JSON] " << result << "\n";
        return fallbackDialogue(role, context);
    }

    return content;
}
