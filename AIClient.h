#ifndef AICLIENT_H
#define AICLIENT_H

#include <string>

class AIClient {
public:
    AIClient() = default;
    ~AIClient() = default;

    // Synchronous call to OpenAI
    std::string generateDialogue(std::string role, std::string context);

private:
    std::string loadApiKey();
    std::string parseJsonContent(const std::string& json);
};

#endif
