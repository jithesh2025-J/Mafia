// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Game.cpp  â€”  Full single-player Mafia game implementation
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
#include "Game.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <cmath>

namespace {

constexpr float kUiSafeMarginPx = 24.f;
constexpr float kUiContainerPaddingPx = 16.f;
constexpr float kUiStackSpacingPx = 22.f;
constexpr float kUiSectionSpacingPx = 24.f;

const sf::Color kThemeBackground(10, 13, 20);
const sf::Color kThemeCard(22, 27, 40);
const sf::Color kThemeAccentBlue(86, 126, 186);
const sf::Color kThemeDangerRed(172, 52, 53);
const sf::Color kThemeTextPrimary(236, 238, 245);
const sf::Color kThemeTextSecondary(170, 179, 195);

const char* kGameVersion = "1.0";
const char* kAiNames[] = {
    "Ava", "Blake", "Cora", "Dante", "Elena",
    "Felix", "Gia", "Hugo", "Iris"
};

const char* kLocations[] = {
    "the alley", "the dock", "the rooftop", "the train station",
    "the market", "the square", "the chapel", "the warehouse"
};
constexpr int kLocationCount = static_cast<int>(sizeof(kLocations) / sizeof(kLocations[0]));

std::string wrapTextToWidth(const sf::Font& font, unsigned int charSize,
                            const std::string& text, float maxWidth);

sf::Color withAlpha(sf::Color color, std::uint8_t alpha)
{
    color.a = alpha;
    return color;
}

float readableLetterSpacing(unsigned int charSize)
{
    if (charSize <= 11) return 1.05f;
    if (charSize <= 16) return 1.08f;
    if (charSize <= 28) return 1.10f;
    return 1.12f;
}

void applyReadableLetterSpacing(sf::Text& text)
{
    text.setLetterSpacing(readableLetterSpacing(text.getCharacterSize()));
}

float snapPixel(float value)
{
    return std::round(value);
}

sf::Vector2f snapPixel(sf::Vector2f value)
{
    return {snapPixel(value.x), snapPixel(value.y)};
}

void snapTextToPixel(sf::Text& text)
{
    applyReadableLetterSpacing(text);
    text.setPosition(snapPixel(text.getPosition()));
}

void placeTextTopLeft(sf::Text& text, float left, float top)
{
    applyReadableLetterSpacing(text);
    const auto bounds = text.getLocalBounds();
    text.setPosition(snapPixel({left - bounds.position.x, top - bounds.position.y}));
}

void placeTextCentered(sf::Text& text, float centerX, float centerY)
{
    applyReadableLetterSpacing(text);
    const auto bounds = text.getLocalBounds();
    text.setPosition(snapPixel({
        centerX - (bounds.position.x + bounds.size.x * 0.5f),
        centerY - (bounds.position.y + bounds.size.y * 0.5f)
    }));
}

sf::FloatRect insetRect(const sf::FloatRect& rect, float padding = kUiContainerPaddingPx)
{
    const float pad = std::max(0.f, padding);
    const float w = std::max(0.f, rect.size.x - pad * 2.f);
    const float h = std::max(0.f, rect.size.y - pad * 2.f);
    return {{rect.position.x + pad, rect.position.y + pad}, {w, h}};
}

void fitTextToWidth(sf::Text& text, float maxWidth, unsigned int minSize = 10)
{
    if (maxWidth <= 0.f) return;
    applyReadableLetterSpacing(text);
    unsigned int size = text.getCharacterSize();
    while (size > minSize && text.getLocalBounds().size.x > maxWidth) {
        --size;
        text.setCharacterSize(size);
        applyReadableLetterSpacing(text);
    }
}

void applyWrappedText(sf::Text& text,
                      const sf::Font& font,
                      unsigned int charSize,
                      const std::string& source,
                      float maxWidth,
                      float lineSpacing = 1.15f)
{
    text.setFont(font);
    text.setCharacterSize(charSize);
    applyReadableLetterSpacing(text);
    text.setString(wrapTextToWidth(font, charSize, source, maxWidth));
    text.setLineSpacing(lineSpacing);
}

void applyWrappedTextFitted(sf::Text& text,
                            const sf::Font& font,
                            unsigned int startSize,
                            const std::string& source,
                            float maxWidth,
                            float maxHeight,
                            unsigned int minSize = 10,
                            float lineSpacing = 1.15f)
{
    unsigned int size = std::max(minSize, startSize);
    while (size >= minSize) {
        applyWrappedText(text, font, size, source, maxWidth, lineSpacing);
        if (text.getLocalBounds().size.y <= maxHeight || size == minSize) {
            break;
        }
        --size;
    }
}

struct VStack {
    float left;
    float width;
    float cursorY;
    float spacing;

    VStack(float x, float y, float w, float gap = kUiStackSpacingPx)
        : left(x), width(w), cursorY(y), spacing(gap) {}

    sf::FloatRect next(float height)
    {
        sf::FloatRect rect({left, cursorY}, {width, std::max(0.f, height)});
        cursorY += rect.size.y + spacing;
        return rect;
    }

    float currentY() const { return cursorY; }
    void setY(float y) { cursorY = y; }
};

std::string spacedLabel(const std::string& text)
{
    std::string result;
    result.reserve(text.size() * 2);
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i != 0) result.push_back(' ');
        result.push_back(text[i]);
    }
    return result;
}

sf::FloatRect makeLetterboxViewport(sf::Vector2u windowSize, sf::Vector2f targetSize)
{
    if (windowSize.x == 0 || windowSize.y == 0 || targetSize.x <= 0.f || targetSize.y <= 0.f) {
        return { {0.f, 0.f}, {1.f, 1.f} };
    }

    const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float targetRatio = targetSize.x / targetSize.y;

    if (windowRatio > targetRatio) {
        const float width = targetRatio / windowRatio;
        const float offsetX = (1.f - width) * 0.5f;
        return { {offsetX, 0.f}, {width, 1.f} };
    }

    const float height = windowRatio / targetRatio;
    const float offsetY = (1.f - height) * 0.5f;
    return { {0.f, offsetY}, {1.f, height} };
}

std::filesystem::path exeDir()
{
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (ec) return {};
    return cwd;
}

bool loadFont(sf::Font& f)
{
    const char* windir = std::getenv("WINDIR");
    std::filesystem::path sys(windir ? windir : "C:\\Windows");
    sys /= "Fonts";

    std::vector<std::filesystem::path> paths;
#ifdef _WIN32
    paths.push_back(sys / "segoeui.ttf");
    paths.push_back(sys / "calibri.ttf");
    paths.push_back(sys / "arial.ttf");
    auto ed = exeDir();
    if (!ed.empty()) {
        paths.push_back(ed / "Xirod.otf.otf");
        paths.push_back(ed / "Xirod.otf");
        paths.push_back(ed / "Xirod.ttf");
    }
#endif
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (!ec) {
        paths.push_back(cwd / "Xirod.otf.otf");
        paths.push_back(cwd / "Xirod.otf");
        paths.push_back(cwd / "Xirod.ttf");
    }

    for (auto& p : paths) {
        if (!p.empty() && f.openFromFile(p.string())) {
            f.setSmooth(true);
            return true;
        }
    }
    return false;
}

std::vector<std::filesystem::path> collectAudioSearchDirs()
{
    std::vector<std::filesystem::path> dirs;
#ifdef _WIN32
    auto ed = exeDir();
    if (!ed.empty()) {
        dirs.push_back(ed);
    }
#endif

    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (!ec) {
        dirs.push_back(cwd);
    }

    dirs.push_back(std::filesystem::path("audio_generated"));
    dirs.push_back(std::filesystem::path("audio_pack"));
    return dirs;
}

bool tryLoadBufferFromNames(sf::SoundBuffer& dst,
                            const std::vector<std::filesystem::path>& dirs,
                            const std::vector<std::string>& names)
{
    for (const auto& dir : dirs) {
        for (const auto& name : names) {
            const auto path = dir / name;
            if (dst.loadFromFile(path.string())) {
                return true;
            }
        }
    }
    return false;
}

std::string replaceAll(std::string s, const std::string& from, const std::string& to)
{
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::vector<std::string> splitByNewline(const std::string& text)
{
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    if (!text.empty() && text.back() == '\n') {
        lines.push_back("");
    }
    return lines;
}

std::string wrapTextToWidth(const sf::Font& font, unsigned int charSize,
                            const std::string& text, float maxWidth)
{
    if (maxWidth <= 0.f || text.empty()) return text;

    std::vector<std::string> wrappedLines;
    std::istringstream paragraphs(text);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph)) {
        if (paragraph.empty()) {
            wrappedLines.push_back("");
            continue;
        }

        std::istringstream words(paragraph);
        std::string word;
        std::string currentLine;
        while (words >> word) {
            std::string trial = currentLine.empty() ? word : currentLine + " " + word;
            sf::Text measure(font, trial, charSize);
            if (measure.getLocalBounds().size.x <= maxWidth || currentLine.empty()) {
                currentLine = trial;
            } else {
                wrappedLines.push_back(currentLine);
                currentLine = word;
            }
        }
        if (!currentLine.empty()) {
            wrappedLines.push_back(currentLine);
        }
    }

    std::ostringstream out;
    for (size_t i = 0; i < wrappedLines.size(); ++i) {
        out << wrappedLines[i];
        if (i + 1 < wrappedLines.size()) out << '\n';
    }
    return out.str();
}

// Generate a role description (two lines)
const char* roleDesc(Player::Role r)
{
    switch (r) {
        case Player::Role::MAFIA:     return "Eliminate town members each night.\nStay hidden and avoid the vote.";
        case Player::Role::GODFATHER: return "Lead the mafia from the shadows.\nDetectives see you as innocent.";
        case Player::Role::SILENCER:  return "Silence one player each night.\nThey cannot speak next discussion.";
        case Player::Role::DETECTIVE: return "Investigate one player each night\nto learn their true role.";
        case Player::Role::DOCTOR:    return "Protect one player each night\nfrom being killed by the mafia.";
        case Player::Role::JOKER:     return "Get yourself voted out\nto win the game!";
        default:                      return "Find and vote out the mafia\nbefore they take over the town.";
    }
}

sf::Color roleCardColor(Player::Role r)
{
    switch (r) {
        case Player::Role::MAFIA:     return sf::Color(139,  0,  0);
        case Player::Role::GODFATHER: return sf::Color(100,  0, 20);
        case Player::Role::SILENCER:  return sf::Color( 80, 20, 60);
        case Player::Role::DETECTIVE: return sf::Color(  0, 48,128);
        case Player::Role::DOCTOR:    return sf::Color(  0, 85,  0);
        case Player::Role::JOKER:     return sf::Color( 64,  0, 96);
        default:                      return sf::Color( 48, 48, 48);
    }
}

bool isMafiaAlignedRole(Player::Role r)
{
    return r == Player::Role::MAFIA ||
           r == Player::Role::GODFATHER ||
           r == Player::Role::SILENCER;
}

float randomRange(float minV, float maxV)
{
    if (maxV <= minV) return minV;
    float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minV + t * (maxV - minV);
}

std::vector<short> synthTone(float seconds, int sampleRate,
                             float freqA, float freqB,
                             float wobbleHz, float noiseMix,
                             float attack, float release,
                             float gain)
{
    const int count = std::max(1, static_cast<int>(seconds * sampleRate));
    std::vector<short> out(count);
    float phaseA = 0.f;
    float phaseB = 0.f;
    unsigned int seed = 0x1234ABCDu;

    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float env = 1.f;
        if (attack > 0.f) env *= std::min(1.f, t / attack);
        float rem = seconds - t;
        if (release > 0.f) env *= std::min(1.f, rem / release);

        float wobble = std::sin(2.f * 3.1415926f * wobbleHz * t);
        float fA = freqA * (1.f + 0.03f * wobble);
        float fB = freqB * (1.f - 0.02f * wobble);
        phaseA += 2.f * 3.1415926f * fA / sampleRate;
        phaseB += 2.f * 3.1415926f * fB / sampleRate;
        if (phaseA > 2.f * 3.1415926f) phaseA -= 2.f * 3.1415926f;
        if (phaseB > 2.f * 3.1415926f) phaseB -= 2.f * 3.1415926f;

        seed = seed * 1664525u + 1013904223u;
        float noise = (static_cast<float>((seed >> 8) & 0xFFFF) / 32768.f) - 1.f;

        float tone = 0.65f * std::sin(phaseA) + 0.35f * std::sin(phaseB);
        float sample = (tone * (1.f - noiseMix) + noise * noiseMix) * env * gain;
        sample = std::clamp(sample, -1.f, 1.f);
        out[i] = static_cast<short>(sample * 32767.f);
    }
    return out;
}

bool writeWavFile(const std::filesystem::path& p, const std::vector<short>& samples, int sampleRate)
{
    std::ofstream f(p, std::ios::binary);
    if (!f.is_open()) return false;

    const int channels = 1;
    const int bitsPerSample = 16;
    const int byteRate = sampleRate * channels * bitsPerSample / 8;
    const int blockAlign = channels * bitsPerSample / 8;
    const int dataSize = static_cast<int>(samples.size() * sizeof(short));
    const int riffSize = 36 + dataSize;

    f.write("RIFF", 4);
    f.write(reinterpret_cast<const char*>(&riffSize), 4);
    f.write("WAVE", 4);
    f.write("fmt ", 4);

    int subchunk1Size = 16;
    short audioFormat = 1;
    short ch = static_cast<short>(channels);
    short bps = static_cast<short>(bitsPerSample);

    f.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    f.write(reinterpret_cast<const char*>(&audioFormat), 2);
    f.write(reinterpret_cast<const char*>(&ch), 2);
    f.write(reinterpret_cast<const char*>(&sampleRate), 4);
    f.write(reinterpret_cast<const char*>(&byteRate), 4);
    f.write(reinterpret_cast<const char*>(&blockAlign), 2);
    f.write(reinterpret_cast<const char*>(&bps), 2);

    f.write("data", 4);
    f.write(reinterpret_cast<const char*>(&dataSize), 4);
    if (!samples.empty())
        f.write(reinterpret_cast<const char*>(samples.data()), dataSize);
    return true;
}

} // namespace

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Constructor / Destructor
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
const sf::Color Game::kColors[10] = {
    sf::Color(120, 180, 255),
    sf::Color(255, 130, 130),
    sf::Color(150, 220, 160),
    sf::Color(220, 180, 110),
    sf::Color(190, 150, 255),
    sf::Color(120, 220, 220),
    sf::Color(255, 170, 120),
    sf::Color(180, 180, 200),
    sf::Color(255, 210, 120),
    sf::Color(140, 190, 255)
};

Game::Game()
    : window(sf::VideoMode::getDesktopMode(), "Mafia Game", sf::Style::None),
    gameView(sf::FloatRect({0.f, 0.f}, {1280.f, 720.f})),
      bgSprite(bgTexture)
{
    srand(static_cast<unsigned>(time(nullptr)));
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(true);
    updateGameView();
    fontLoaded = loadFont(font);
    if (fontLoaded) {
        font.setSmooth(true);
    }
    if (!fontLoaded)
        std::cerr << "Game: could not load any font.\n";

    bgLoaded = false;
    const std::filesystem::path menuBgPath("background1.jpg");
    std::error_code bgEc;
    if (std::filesystem::exists(menuBgPath, bgEc) && !bgEc) {
        bgLoaded = bgTexture.loadFromFile(menuBgPath.string());
        if (bgLoaded) {
            bgSprite.setTexture(bgTexture);
            sf::Vector2u size = bgTexture.getSize();
            if (size.x >= 1280u && size.y >= 720u) {
                int left = static_cast<int>((size.x - 1280u) / 2u);
                int top = static_cast<int>((size.y - 720u) / 2u);
                bgSprite.setTextureRect(sf::IntRect({left, top}, {1280, 720}));
            }
            bgSprite.setPosition({0.f, 0.f});
        }
    }

    loadStats();
    initAudio();
    initNightSky();
    startMainMenu();
}

Game::~Game() = default;

void Game::initAudio()
{
    const int sr = 22050;
    std::error_code ec;
    std::filesystem::path root = std::filesystem::current_path(ec);
    if (ec) return;
    std::filesystem::path dir = root / "audio_generated";
    std::filesystem::create_directories(dir, ec);
    if (ec) return;

    auto click = synthTone(0.065f, sr, 1300.f, 880.f, 15.f, 0.03f, 0.002f, 0.03f, 0.42f);
    auto confirm = synthTone(0.14f, sr, 520.f, 780.f, 4.f, 0.01f, 0.005f, 0.08f, 0.46f);
    auto death = synthTone(0.52f, sr, 220.f, 92.f, 2.5f, 0.06f, 0.01f, 0.35f, 0.55f);
    auto tick = synthTone(0.045f, sr, 1600.f, 1200.f, 10.f, 0.02f, 0.001f, 0.02f, 0.36f);
    auto phase = synthTone(0.21f, sr, 340.f, 510.f, 1.5f, 0.015f, 0.01f, 0.12f, 0.42f);
    auto chatOpen = synthTone(0.08f, sr, 940.f, 1310.f, 8.f, 0.02f, 0.002f, 0.04f, 0.40f);
    auto ambMenu = synthTone(6.5f, sr, 84.f, 126.f, 0.35f, 0.08f, 1.5f, 1.5f, 0.25f);
    auto ambGame = synthTone(6.5f, sr, 67.f, 102.f, 0.42f, 0.10f, 1.5f, 1.5f, 0.30f);

    const auto clickP = dir / "ui_click.wav";
    const auto confirmP = dir / "ui_confirm.wav";
    const auto deathP = dir / "death_event.wav";
    const auto tickP = dir / "vote_tick.wav";
    const auto phaseP = dir / "phase_shift.wav";
    const auto chatOpenP = dir / "chat_open.wav";
    const auto ambMenuP = dir / "ambient_menu.wav";
    const auto ambGameP = dir / "ambient_game.wav";

    if (!writeWavFile(clickP, click, sr)) return;
    writeWavFile(confirmP, confirm, sr);
    writeWavFile(deathP, death, sr);
    writeWavFile(tickP, tick, sr);
    writeWavFile(phaseP, phase, sr);
    writeWavFile(chatOpenP, chatOpen, sr);
    writeWavFile(ambMenuP, ambMenu, sr);
    writeWavFile(ambGameP, ambGame, sr);

    audioEnabled = sfxClickBuffer.loadFromFile(clickP.string()) &&
                   sfxConfirmBuffer.loadFromFile(confirmP.string()) &&
                   sfxDeathBuffer.loadFromFile(deathP.string()) &&
                   sfxVoteTickBuffer.loadFromFile(tickP.string()) &&
                   sfxPhaseBuffer.loadFromFile(phaseP.string()) &&
                   sfxChatOpenBuffer.loadFromFile(chatOpenP.string()) &&
                   ambientMenuBuffer.loadFromFile(ambMenuP.string()) &&
                   ambientGameBuffer.loadFromFile(ambGameP.string());
    if (!audioEnabled) return;

    // Optional premium pack override: place files in ./audio_pack or ./audio.
    const auto searchDirs = collectAudioSearchDirs();
    auto overrideIfExists = [&](sf::SoundBuffer& dst, const std::vector<std::string>& names) {
        sf::SoundBuffer custom;
        if (tryLoadBufferFromNames(custom, searchDirs, names)) dst = std::move(custom);
    };

    overrideIfExists(sfxClickBuffer, {"ui_click", "click", "button_click"});
    overrideIfExists(sfxConfirmBuffer, {"ui_confirm", "confirm", "button_confirm"});
    overrideIfExists(sfxDeathBuffer, {"kill", "kill_stab", "death_event"});
    overrideIfExists(sfxVoteTickBuffer, {"vote_tick", "voting_tick", "vote_reveal"});
    overrideIfExists(sfxPhaseBuffer, {"phase_shift", "phase_transition", "round_transition"});
    overrideIfExists(sfxChatOpenBuffer, {"chat_open", "ui_chat_open", "chat_notification"});
    overrideIfExists(ambientMenuBuffer, {"ambient_menu", "menu_loop", "menu_ambient"});
    overrideIfExists(ambientGameBuffer, {"ambient_game", "game_loop", "match_ambient"});

    sfxClick = std::make_unique<sf::Sound>(sfxClickBuffer);
    sfxConfirm = std::make_unique<sf::Sound>(sfxConfirmBuffer);
    sfxDeath = std::make_unique<sf::Sound>(sfxDeathBuffer);
    sfxVoteTick = std::make_unique<sf::Sound>(sfxVoteTickBuffer);
    sfxPhase = std::make_unique<sf::Sound>(sfxPhaseBuffer);
    sfxChatOpen = std::make_unique<sf::Sound>(sfxChatOpenBuffer);
    ambientMenuSound = std::make_unique<sf::Sound>(ambientMenuBuffer);
    ambientGameSound = std::make_unique<sf::Sound>(ambientGameBuffer);
    ambientMenuSound->setLooping(true);
    ambientGameSound->setLooping(true);

    sfxClick->setVolume(35.f);
    sfxConfirm->setVolume(45.f);
    sfxDeath->setVolume(62.f);
    sfxVoteTick->setVolume(28.f);
    sfxPhase->setVolume(40.f);
    sfxChatOpen->setVolume(34.f);

    ambientMenuCurrentVol = 0.f;
    ambientGameCurrentVol = 0.f;
    ambientMenuSound->setVolume(0.f);
    ambientGameSound->setVolume(0.f);
    ambientMenuSound->play();
    ambientGameSound->play();
}

void Game::playSfx(sf::Sound* sfx, float volume, bool isUi)
{
    if (!audioEnabled || sfx == nullptr) return;
    float bus = isUi ? uiVolume : sfxVolume;
    float effective = volume * (masterVolume / 100.f) * (bus / 100.f);
    sfx->stop();
    sfx->setVolume(std::clamp(effective, 0.f, 100.f));
    sfx->play();
}

void Game::updateAmbientForPhase()
{
    if (!audioEnabled || !ambientMenuSound || !ambientGameSound) return;

    bool menuLike = (phase == Phase::MAIN_MENU || phase == Phase::SETTINGS || phase == Phase::LOBBY);
    float menuTargetVol = menuLike ? 20.f : 0.f;
    float gameTargetVol = menuLike ? 0.f : 30.f;

    float phaseIntensity = 1.f;
    if (phase == Phase::NIGHT) phaseIntensity = 1.08f;
    else if (phase == Phase::VOTING) phaseIntensity = 1.22f;
    else if (phase == Phase::DISCUSSION) {
        float left = std::clamp(1.f - discClock.getElapsedTime().asSeconds() / std::max(1.f, discussionDurationSec), 0.f, 1.f);
        phaseIntensity = 0.95f + (1.f - left) * 0.28f;
    } else if (phase == Phase::GAME_OVER) phaseIntensity = 1.15f;

    if (quickMenuOpen) phaseIntensity *= 0.62f;

    menuTargetVol *= (masterVolume / 100.f) * (ambientVolume / 100.f) * phaseIntensity;
    gameTargetVol *= (masterVolume / 100.f) * (ambientVolume / 100.f) * phaseIntensity;

    const float smooth = 0.08f;
    ambientMenuCurrentVol += (menuTargetVol - ambientMenuCurrentVol) * smooth;
    ambientGameCurrentVol += (gameTargetVol - ambientGameCurrentVol) * smooth;

    ambientMenuSound->setVolume(std::clamp(ambientMenuCurrentVol, 0.f, 100.f));
    ambientGameSound->setVolume(std::clamp(ambientGameCurrentVol, 0.f, 100.f));

    if (ambientMenuSound->getStatus() != sf::SoundSource::Status::Playing) ambientMenuSound->play();
    if (ambientGameSound->getStatus() != sf::SoundSource::Status::Playing) ambientGameSound->play();
}

void Game::toggleMute()
{
    if (!audioMuted) {
        masterVolumeBeforeMute = std::clamp(masterVolume, 0.f, 100.f);
        if (masterVolumeBeforeMute < 1.f) masterVolumeBeforeMute = 100.f;
        masterVolume = 0.f;
        audioMuted = true;
    } else {
        masterVolume = std::clamp(masterVolumeBeforeMute, 0.f, 100.f);
        audioMuted = false;
    }
    saveStats();
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Main loop
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::run()
{
    while (window.isOpen()) {
        handleEvents();
        update();
        render();
    }
}

void Game::updateGameView()
{
    gameView = sf::View(sf::FloatRect({0.f, 0.f}, {1280.f, 720.f}));
    gameView.setViewport(makeLetterboxViewport(window.getSize(), {1280.f, 720.f}));
    window.setView(gameView);
}

sf::FloatRect Game::uiSafeArea(float marginPx) const
{
    const float safe = std::max(0.f, marginPx);
    const float vw = gameView.getSize().x;
    const float vh = gameView.getSize().y;
    return sf::FloatRect({safe, safe}, {std::max(0.f, vw - safe * 2.f), std::max(0.f, vh - safe * 2.f)});
}

sf::FloatRect Game::clampToSafeArea(const sf::FloatRect& r, float insetPx) const
{
    sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx + std::max(0.f, insetPx));
    sf::FloatRect out = r;
    out.size.x  = std::min(std::max(0.f, out.size.x), safe.size.x);
    out.size.y = std::min(std::max(0.f, out.size.y), safe.size.y);
    out.position.x   = std::clamp(out.position.x, safe.position.x, safe.position.x + safe.size.x - out.size.x);
    out.position.y    = std::clamp(out.position.y, safe.position.y, safe.position.y + safe.size.y - out.size.y);
    return out;
}

float Game::uiX(float pct) const
{
    return gameView.getSize().x * pct;
}

float Game::uiY(float pct) const
{
    return gameView.getSize().y * pct;
}

sf::FloatRect Game::uiRect(float xPct, float yPct, float wPct, float hPct) const
{
    const sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);
    sf::FloatRect rect(
        {safe.position.x + safe.size.x * xPct, safe.position.y + safe.size.y * yPct},
        {safe.size.x * wPct, safe.size.y * hPct}
    );
    return clampToSafeArea(rect);
}

unsigned int Game::uiFont(unsigned int baseSize) const
{
    return std::max(10u, baseSize);
}

void Game::drawDebugBounds(const sf::FloatRect& rect, sf::Color color, float thickness)
{
    if (!uiDebugMode) return;
    sf::RectangleShape outline({std::max(0.f, rect.size.x), std::max(0.f, rect.size.y)});
    outline.setPosition({rect.position.x, rect.position.y});
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineColor(color);
    outline.setOutlineThickness(thickness);
    window.draw(outline);
}

void Game::drawDebugTextBounds(const sf::Text& text, sf::Color color)
{
    if (!uiDebugMode) return;
    drawDebugBounds(text.getGlobalBounds(), color, 1.f);
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  handleEvents
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::handleEvents()
{
    mouseClicked = false;
    window.setView(gameView);
    mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    auto processEvent = [&](const sf::Event& event) {
        if (event.is<sf::Event::Closed>()) { window.close(); return false; }

        if (event.is<sf::Event::Resized>()) {
            updateGameView();
        }

        // Left click
        if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mb->button == sf::Mouse::Button::Left) {
                clickPos = window.mapPixelToCoords(mb->position);
                mouseClicked = true;
            }
        }

        if (const auto* mw = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (phase == Phase::INSTRUCTIONS && !quickMenuOpen) {
                instructionsScroll -= mw->delta * 42.f; // Adjust scroll based on mouse wheel
            }
        }

        // Text input (lobby name field + discussion chat)
        if (const auto* te = event.getIf<sf::Event::TextEntered>()) {
            auto ch = te->unicode;
            if (quickMenuOpen) return true;
            if (phase == Phase::LOBBY && nameFocused) {
                if (ch == '\b') { if (!humanName.empty()) humanName.pop_back(); }
                else if (ch >= 32 && ch < 127 && humanName.size() < 16)
                    humanName += static_cast<char>(ch);
            }
            if (phase == Phase::DISCUSSION && !players[humanIdx].isSilenced() && !lastWillOpen) {
                if (ch == '\b') { if (!chatInput.empty()) chatInput.pop_back(); }
                else if (ch >= 32 && ch < 127 && chatInput.size() < 60)
                    chatInput += static_cast<char>(ch);
            }
            if (lastWillOpen && humanIdx < (int)lastWills.size()) {
                if (ch == '\b') { if (!lastWills[humanIdx].empty()) lastWills[humanIdx].pop_back(); }
                else if (ch >= 32 && ch < 127 && lastWills[humanIdx].size() < 120)
                    lastWills[humanIdx] += static_cast<char>(ch);
            }
        }

        // Key pressed
        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::F1) {
                uiDebugMode = !uiDebugMode;
            }
            if (kp->code == sf::Keyboard::Key::F9) {
                toggleMute();
            }
            if (kp->code == sf::Keyboard::Key::Escape) {
                const bool inMatch = (phase == Phase::ROLE_REVEAL || phase == Phase::NIGHT ||
                                      phase == Phase::DAY || phase == Phase::DISCUSSION ||
                                      phase == Phase::VOTING || phase == Phase::GAME_OVER);
                if (quickMenuOpen) {
                    quickMenuOpen = false;
                } else if (inMatch) {
                    quickMenuOpen = true;
                    lastWillOpen = false;
                    graveyardOpen = false;
                    journalOpen = false;
                } else if (phase == Phase::SETTINGS || phase == Phase::INSTRUCTIONS || phase == Phase::LOBBY) {
                    transitionTo(Phase::MAIN_MENU);
                }
            }
            if (quickMenuOpen && kp->code == sf::Keyboard::Key::M) {
                toggleMute();
            }
            if (!quickMenuOpen && kp->code == sf::Keyboard::Key::Tab) {
                if (phase == Phase::DISCUSSION || phase == Phase::VOTING || phase == Phase::DAY)
                    journalOpen = !journalOpen;
            }
            if (!quickMenuOpen && kp->code == sf::Keyboard::Key::W) {
                if (phase == Phase::NIGHT || phase == Phase::DAY ||
                    phase == Phase::DISCUSSION || phase == Phase::VOTING) {
                    lastWillOpen = !lastWillOpen;
                    graveyardOpen = false;
                    journalOpen = false;
                }
            }
            if (!quickMenuOpen && kp->code == sf::Keyboard::Key::G) {
                if (phase == Phase::DAY || phase == Phase::DISCUSSION || phase == Phase::VOTING) {
                    graveyardOpen = !graveyardOpen;
                    lastWillOpen = false;
                    journalOpen = false;
                }
            }
            if (!quickMenuOpen && kp->code == sf::Keyboard::Key::Enter) {
                if (phase == Phase::DISCUSSION && !chatInput.empty()) {
                    std::lock_guard<std::mutex> lock(chatMutex);
                    for (int i = 0; i < (int)players.size(); i++) {
                        if (chatInput.find(players[i].getName()) != std::string::npos) {
                            accusations[i]++;
                            if (i < (int)publicSuspicion.size())
                                publicSuspicion[i] = std::clamp(publicSuspicion[i] + 1.8f, 0.f, 100.f);
                            for (int observer = 1; observer < (int)players.size(); ++observer) {
                                if (!players[observer].getIsAlive() || players[observer].getIsHuman()) continue;
                                applySuspicionDelta(observer, i, 1.2f);
                            }
                        }
                    }
                    chat.push_back({ players[humanIdx].getName(), chatInput, sf::Color::White });
                    chatInput.clear();
                    playSfx(sfxChatOpen.get(), 32.f, true);
                }
            }
        }
        return true;
    };

    // Block input during transitions
    if (transitioning) {
        while (const auto event = window.pollEvent()) {
            if (!processEvent(*event)) return;
        }
        return;
    }

    while (const auto event = window.pollEvent()) {
        if (!processEvent(*event)) return;
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  update
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::update()
{
    float dt = frameClock.restart().asSeconds();
    if (transitioning) { updateTransition(); return; }
    updateAmbientForPhase();
    updateParticles(dt);
    if (quickMenuOpen) return;

    switch (phase) {
        case Phase::ROLE_REVEAL:  updateRoleReveal();  break;
        case Phase::NIGHT:        updateNight();        break;
        case Phase::DISCUSSION:   updateDiscussion();   break;
        case Phase::VOTING:       updateVoting();       break;
        default: break;
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  render
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::render()
{
    window.clear(kThemeBackground);
    window.setView(gameView);

    switch (phase) {
        case Phase::MAIN_MENU:   renderMainMenu();    break;
        case Phase::SETTINGS:    renderSettings();    break;
        case Phase::INSTRUCTIONS: renderInstructions(); break;
        case Phase::LOBBY:       renderLobby();       break;
        case Phase::ROLE_REVEAL: renderRoleReveal();  break;
        case Phase::NIGHT:       renderNight();       break;
        case Phase::DAY:         renderDay();         break;
        case Phase::DISCUSSION:  renderDiscussion();  break;
        case Phase::VOTING:      renderVoting();      break;
        case Phase::GAME_OVER:   renderGameOver();    break;
    }

    // Overlay layers
    renderParticles();
    if (journalOpen) renderJournal();
    if (graveyardOpen) renderGraveyard();
    if (lastWillOpen) renderLastWillPopup();
    if (quickMenuOpen) renderQuickMenu();
    renderTooltip();
    renderTransitionOverlay();

    if (uiDebugMode && fontLoaded) {
        sf::RectangleShape hud({360.f, 58.f});
        hud.setPosition({16.f, 656.f});
        hud.setFillColor(sf::Color(10, 10, 16, 190));
        hud.setOutlineColor(sf::Color(120, 140, 200));
        hud.setOutlineThickness(1.f);
        window.draw(hud);

        sf::Text title(font, "UI DEBUG (F1): text=green panel=cyan card=magenta", 13);
        title.setFillColor(sf::Color(220, 230, 250));
        title.setPosition({24.f, 664.f});
        window.draw(title);

        sf::Text line2(font, "button bounds=gold", 12);
        line2.setFillColor(sf::Color(245, 215, 120));
        line2.setPosition({24.f, 686.f});
        window.draw(line2);
    }

    // Keep stable non-shaken view for input mapping between frames.
    window.setView(gameView);
    window.display();
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Phase transitions
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::startMainMenu()
{
    resetGame();
    phase = Phase::MAIN_MENU;
    phaseAnimClock.restart();
    updateAmbientForPhase();
}

void Game::startSettings()
{
    phase = Phase::SETTINGS;
    phaseAnimClock.restart();
    updateAmbientForPhase();
}

void Game::startInstructions()
{
    phase = Phase::INSTRUCTIONS;
    phaseAnimClock.restart();
    updateAmbientForPhase();
}

void Game::startLobby()
{
    resetGame();
    phase = Phase::LOBBY;
    humanName.clear();
    nameFocused = false;
    phaseAnimClock.restart();
    updateAmbientForPhase();
}

void Game::startRoleReveal()
{
    phase = Phase::ROLE_REVEAL;
    cardY = 800.f;
    blinkState = false;
    roleRevealClock.restart();
    blinkClock.restart();
    phaseAnimClock.restart();
    updateAmbientForPhase();
    playSfx(sfxPhase.get(), 38.f);
}

void Game::startNight()
{
    phase = Phase::NIGHT;
    nightHumanTarget  = -1;
    nightConfirmed    = false;
    detectPopupOpen   = false;
    detectRevealIdx   = -1;
    silencerTarget    = -1;
    nightClock.restart();
    zzzClock.restart();
    // Unsilence everyone at start of new night
    for (auto& p : players) p.unsilence();
    silencedPlayerIdx = -1;
    phaseAnimClock.restart();
    updateAmbientForPhase();
    playSfx(sfxPhase.get(), 34.f);
}

void Game::startDay(const std::string& announcement)
{
    phase  = Phase::DAY;
    dayMsg = announcement;
    journalOpen = false;
    generateEvidence();
    phaseAnimClock.restart();
    updateAmbientForPhase();
    playSfx(sfxPhase.get(), 36.f);
}

void Game::startDiscussion()
{
    phase = Phase::DISCUSSION;
    chatInput.clear();
    for (auto it = accusations.begin(); it != accusations.end();) {
        it->second = std::max(0, it->second - 1);
        if (it->second == 0) it = accusations.erase(it);
        else ++it;
    }
    decayAiSuspicion();
    journalOpen = false;
    discClock.restart();
    aiChatClock.restart();
    nextAiDelay = randomRange(aiChatMinDelaySec, aiChatMaxDelaySec);
    phaseAnimClock.restart();
    updateAmbientForPhase();
    playSfx(sfxPhase.get(), 30.f);
    playSfx(sfxChatOpen.get(), 28.f, true);
}

void Game::startVoting()
{
    phase           = Phase::VOTING;
    humanVoteTarget = -1;
    humanVoted      = false;
    showConfirm     = false;
    votesDone       = false;
    eliminatedIdx   = -1;
    showElim        = false;
    elimContinueReady = false;
    tallyAnim       = false;
    journalOpen     = false;
    graveyardOpen   = false;
    lastWillOpen    = false;
    revealingVotes  = false;
    voteRevealIdx   = 0;
    voteReveals.clear();
    roundVotes.clear();
    voteCounts.assign(players.size(), 0);
    tallyShown.assign(players.size(), 0);
    phaseAnimClock.restart();
    updateAmbientForPhase();
    playSfx(sfxPhase.get(), 42.f);
    playSfx(sfxConfirm.get(), 24.f, true);
}

void Game::endGame(const std::string& faction, bool joker)
{
    phase       = Phase::GAME_OVER;
    winFaction  = faction;
    jokerWon    = joker;
    phaseAnimClock.restart();
    updateAmbientForPhase();
    playSfx(sfxPhase.get(), 50.f);
    // Update stats
    stats.gamesPlayed++;
    Player& h = players[humanIdx];
    bool humanWon = false;
    if (faction == "Mafia"  && (h.getRole()==Player::Role::MAFIA ||
                                h.getRole()==Player::Role::GODFATHER ||
                                h.getRole()==Player::Role::SILENCER)) humanWon=true;
    if (faction == "Town"   && h.getRole()!=Player::Role::MAFIA &&
                                h.getRole()!=Player::Role::GODFATHER &&
                                h.getRole()!=Player::Role::SILENCER) humanWon=true;
    if (faction == "Joker"  && h.getRole()==Player::Role::JOKER) humanWon=true;
    if (humanWon) {
        stats.wins++;
        if (faction=="Mafia") stats.mafiaWins++;
        if (faction=="Town")  stats.townWins++;
        if (faction=="Joker") stats.jokerWins++;
        spawnParticles({640.f, 200.f}, sf::Color::White, 60, true);
    } else {
        stats.losses++;
    }
    saveStats();
}

void Game::resetGame()
{
    std::lock_guard<std::mutex> lock(chatMutex);
    players.clear();
    chat.clear();
    chatInput.clear();
    clues.clear();
    aiInvestigated.clear();
    accusations.clear();
    voteCounts.clear();
    tallyShown.clear();
    evidence.clear();
    voteHistory.clear();
    killLog.clear();
    aiPersonalities.clear();
    roundVotes.clear();
    particles.clear();
    lastWills.clear();
    voteReveals.clear();
    aiSuspicion.clear();
    publicSuspicion.clear();
    publicCredibility.clear();
    aiLastVoteTarget.clear();
    lastDoctorSavedTarget = -1;
    lastKilledIdx = -1;
    humanIdx      = 0;
    roundNumber   = 1;
    journalOpen   = false;
    graveyardOpen = false;
    lastWillOpen  = false;
    quickMenuOpen = false;
    silencedPlayerIdx = -1;
    hoveredCardIdx = -1;
    hoverTime      = 0.f;
    voteRevealIdx  = 0;
    revealingVotes = false;
    winFaction.clear();
    jokerWon = false;
}

void Game::renderQuickMenu()
{
    sf::RectangleShape dim({1280.f, 720.f});
    dim.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(dim);

    const sf::FloatRect quickRect({470.f, 220.f}, {340.f, 290.f});
    const sf::FloatRect quickInner = insetRect(quickRect, 14.f);
    drawPanel(quickRect,
              sf::Color(20, 24, 34, 238), sf::Color(160, 170, 210, 130));

    if (fontLoaded) {
        sf::Text title(font, "", 28);
        title.setFillColor(sf::Color(235, 240, 255));
        applyWrappedTextFitted(title, font, 28, "QUICK MENU", quickInner.size.x, 40.f, 18, 1.1f);
        auto b = title.getLocalBounds();
        title.setPosition({640.f - b.size.x/2.f - b.position.x, quickInner.position.y});
        snapTextToPixel(title);
        window.draw(title);
    }

    sf::FloatRect resumeBtn({520.f, 305.f}, {240.f, 50.f});
    sf::FloatRect homeBtn({520.f, 370.f}, {240.f, 50.f});
    sf::FloatRect exitBtn({520.f, 435.f}, {240.f, 50.f});

    drawButton(resumeBtn, "RESUME", sf::Color(45, 95, 55));
    drawButton(homeBtn, "HOME", sf::Color(65, 85, 130));
    drawButton(exitBtn, "EXIT GAME", sf::Color(120, 45, 45));

    if (fontLoaded) {
        std::string snd = std::string("M: ") + (audioMuted ? "Unmute" : "Mute")
                        + "   (F9 quick toggle)";
        sf::Text hint(font, "", 14);
        hint.setFillColor(sf::Color(205, 215, 230));
        const float hintTop = exitBtn.position.y + exitBtn.size.y + 8.f;
        const float hintMaxH = std::max(12.f, quickRect.position.y + quickRect.size.y - hintTop - 8.f);
        applyWrappedTextFitted(hint, font, 14, snd, quickInner.size.x, hintMaxH, 10, 1.08f);
        auto b = hint.getLocalBounds();
        hint.setPosition({640.f - b.size.x / 2.f - b.position.x, hintTop});
        snapTextToPixel(hint);
        window.draw(hint);
    }

    if (btnClicked(resumeBtn)) {
        quickMenuOpen = false;
    }
    if (btnClicked(homeBtn)) {
        quickMenuOpen = false;
        transitionTo(Phase::MAIN_MENU);
    }
    if (btnClicked(exitBtn)) {
        window.close();
    }

    sf::Text version(font, std::string("Mafia ") + kGameVersion, 16);
    version.setFillColor(sf::Color(170, 180, 205));
    auto vb = version.getLocalBounds();
    version.setPosition({1260.f - vb.size.x - vb.position.x, 688.f - vb.position.y});
    window.draw(version);
}

void Game::assignRoles()
{
    int n         = lobbyCount;
    int mafiaNum  = std::min(n / 3, 2);
    if (mafiaNum < 1) mafiaNum = 1;

    std::vector<Player::Role> roles;
    // Build mafia team
    if (lobbyGodfather && mafiaNum >= 1) {
        roles.push_back(Player::Role::GODFATHER);
        for (int i = 1; i < mafiaNum; ++i) roles.push_back(Player::Role::MAFIA);
    } else {
        for (int i = 0; i < mafiaNum; ++i) roles.push_back(Player::Role::MAFIA);
    }
    if (lobbySilencer) roles.push_back(Player::Role::SILENCER);
    if (lobbyDet)   roles.push_back(Player::Role::DETECTIVE);
    if (lobbyDoc)   roles.push_back(Player::Role::DOCTOR);
    if (lobbyJoker) roles.push_back(Player::Role::JOKER);
    while ((int)roles.size() < n) roles.push_back(Player::Role::VILLAGER);
    // Trim if too many special roles
    while ((int)roles.size() > n) roles.pop_back();

    std::mt19937 rng(std::random_device{}());
    std::shuffle(roles.begin(), roles.end(), rng);

    players.resize(n);
    players[0].setName(humanName.empty() ? "You" : humanName);
    players[0].setHuman(true);
    players[0].setRole(roles[0]);

    for (int i = 1; i < n; ++i) {
        players[i].setName(kAiNames[i - 1]);
        players[i].setHuman(false);
        players[i].setRole(roles[i]);
    }
    humanIdx = 0;

    // Assign AI personalities
    aiPersonalities.resize(n);
    aiPersonalities[0] = AiPersonality::AGGRESSIVE; // unused for human
    for (int i = 1; i < n; ++i) {
        aiPersonalities[i] = static_cast<AiPersonality>(rand() % 5);
    }

    // Initialize last wills
    lastWills.assign(n, "");

    initAiSuspicion();
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Phase updates
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::updateRoleReveal()
{
    float t = roleRevealClock.getElapsedTime().asSeconds();

    // Slide card in after 1.5 s
    if (t > 1.5f) {
        float target = 230.f;
        cardY += (target - cardY) * 0.15f;
        if (cardY < target + 1.f) cardY = target;
    }

    // Blink "click to continue"
    if (t > 1.5f && blinkClock.getElapsedTime().asSeconds() > 0.6f) {
        blinkState = !blinkState;
        blinkClock.restart();
    }

    // Advance on click with cross-fade transition
    if (mouseClicked && t > 1.5f) transitionTo(Phase::NIGHT);
}

void Game::updateNight()
{
    float elapsed = nightClock.getElapsedTime().asSeconds();

    // Timer expired â†’ auto-resolve
    if (elapsed >= nightDurationSec && !nightConfirmed) {
        Player& h = players[humanIdx];
        if (h.getRole() == Player::Role::MAFIA || h.getRole() == Player::Role::GODFATHER) {
            // Pick random non-mafia-aligned target
            std::vector<int> tgts;
            for (int i = 0; i < (int)players.size(); ++i) {
                if (players[i].getIsAlive() && i != humanIdx &&
                    !isMafiaAlignedRole(players[i].getRole()))
                    tgts.push_back(i);
            }
            if (!tgts.empty()) nightHumanTarget = tgts[rand() % tgts.size()];
        } else if (h.getRole() == Player::Role::DOCTOR) {
            nightHumanTarget = humanIdx;  // save self
        } else if (h.getRole() == Player::Role::DETECTIVE) {
            std::vector<int> tgts;
            for (int i = 0; i < (int)players.size(); ++i) {
                if (players[i].getIsAlive() && i != humanIdx) tgts.push_back(i);
            }
            if (!tgts.empty()) {
                nightHumanTarget = tgts[rand() % tgts.size()];
                clues.push_back({ players[nightHumanTarget].getName(),
                                   players[nightHumanTarget].getRole() });
            }
        } else if (h.getRole() == Player::Role::SILENCER) {
            std::vector<int> tgts;
            for (int i = 0; i < (int)players.size(); ++i) {
                if (players[i].getIsAlive() && i != humanIdx &&
                    !isMafiaAlignedRole(players[i].getRole()))
                    tgts.push_back(i);
            }
            if (!tgts.empty()) silencerTarget = tgts[rand() % tgts.size()];
        }
        nightConfirmed = true;
        resolveNight();
    }
}

void Game::updateDiscussion()
{
    // 60-second timer auto-advance
    if (discClock.getElapsedTime().asSeconds() >= discussionDurationSec) {
        transitionTo(Phase::VOTING);
        return;
    }

    // AI chat messages
    if (!aiIsThinking && aiChatClock.getElapsedTime().asSeconds() >= nextAiDelay) {
        postAiChat();
        aiChatClock.restart();
        nextAiDelay = randomRange(aiChatMinDelaySec, aiChatMaxDelaySec);
    }
}

void Game::updateVoting()
{
    if (!humanVoted) return;

    // Process AI votes once â€” store reveals but don't tally yet
    if (!votesDone) {
        processAiVotes();
        votesDone      = true;
        revealingVotes = true;
        voteRevealIdx  = 0;
        voteRevealClock.restart();
    }

    // Sequential vote reveal: one vote every 0.6 seconds
    if (revealingVotes) {
        if (voteRevealClock.getElapsedTime().asSeconds() >= 0.6f &&
            voteRevealIdx < (int)voteReveals.size()) {
            // Reveal this vote â€” add to visual tally
            auto& vr = voteReveals[voteRevealIdx];
            voteCounts[vr.target]++;
            tallyShown[vr.target] = voteCounts[vr.target];
            voteRevealIdx++;
            voteRevealClock.restart();
            // Small shake on each reveal
            triggerShake(2.f);
            if (sfxVoteTick) {
                sfxVoteTick->setPitch(1.f + std::min(0.28f, voteRevealIdx * 0.025f));
            }
            playSfx(sfxVoteTick.get(), 26.f);
        }
        // All votes revealed
        if (voteRevealIdx >= (int)voteReveals.size() &&
            voteRevealClock.getElapsedTime().asSeconds() >= 0.8f) {
            revealingVotes = false;
            eliminatedIdx = findMajorityVote();
            showElim = true;
            elimClock.restart();
            tallyAnim = true; // Delay final elimination reveal for dramatic buildup.
        }
    }

    // Delay before showing final elimination result.
    if (showElim && tallyAnim && elimClock.getElapsedTime().asSeconds() >= 1.2f) {
        tallyAnim = false;

        // Record vote history once result locks in.
        voteHistory.push_back({roundNumber, roundVotes, eliminatedIdx});
        registerVoteSuspicionSignals(eliminatedIdx);

        if (eliminatedIdx >= 0) {
            players[eliminatedIdx].die();
            killLog.push_back({roundNumber, players[eliminatedIdx].getName(), "Voted out"});
            if (!players[eliminatedIdx].getIsHuman() &&
                (eliminatedIdx >= (int)lastWills.size() || lastWills[eliminatedIdx].empty())) {
                if (eliminatedIdx < (int)lastWills.size())
                    lastWills[eliminatedIdx] = generateAiLastWill(eliminatedIdx);
            }

            triggerShake(16.f);
            spawnParticles({640.f, 360.f}, sf::Color(220, 65, 65), 55);
            playSfx(sfxDeath.get(), 65.f);
            playSfx(sfxPhase.get(), 24.f);
        } else {
            playSfx(sfxPhase.get(), 28.f);
        }
    }

    // Allow Continue after 3 s of result
    if (showElim && !tallyAnim && elimClock.getElapsedTime().asSeconds() > 3.2f)
        elimContinueReady = true;
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Draw helpers
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::drawPlayerCard(int idx, float x, float y,
                           bool selected, bool hovered,
                           int voteCount, bool showVotes)
{
    const Player& p = players[idx];
    bool alive = p.getIsAlive();
    float t = uiClock.getElapsedTime().asSeconds();

    static std::unordered_map<int, float> hoverAnim;
    static std::unordered_map<int, float> selectAnim;
    float& cardHoverT = hoverAnim[idx];
    float& cardSelectT = selectAnim[idx];
    cardHoverT += ((hovered ? 1.f : 0.f) - cardHoverT) * 0.22f;
    cardSelectT += ((selected ? 1.f : 0.f) - cardSelectT) * 0.20f;
    float hoverLift = -2.0f * cardHoverT;
    float cardScale = 1.f - 0.012f * cardSelectT;
    float cardW = 200.f * cardScale;
    float cardH = 90.f * cardScale;
    float cardX = x + (200.f - cardW) * 0.5f;
    float cardY = y + hoverLift + (90.f - cardH) * 0.5f;

    // Soft card shadow for depth
    sf::RectangleShape shadow({cardW + 6.f, cardH + 6.f});
    shadow.setPosition({cardX + 3.f, cardY + 4.f});
    shadow.setFillColor(sf::Color(0, 0, 0, 90));
    window.draw(shadow);

    sf::RectangleShape card({cardW, cardH});
    card.setPosition({cardX, cardY});
    card.setOutlineThickness(2.f);

    if (!alive) {
        // Dead players are intentionally dimmed and de-emphasized.
        card.setFillColor(sf::Color(14, 18, 28, 220));
        card.setOutlineColor(sf::Color(120, 46, 46));
    } else if (selected) {
        float pulse = 0.5f + 0.5f * std::sin(t * 5.f + idx * 0.6f);
        card.setFillColor(sf::Color(
            static_cast<std::uint8_t>(80 + pulse * 30),
            static_cast<std::uint8_t>(28 + pulse * 10),
            static_cast<std::uint8_t>(34 + pulse * 14)
        ));
        card.setOutlineColor(sf::Color(
            static_cast<std::uint8_t>(220 + pulse * 20),
            static_cast<std::uint8_t>(60 + pulse * 18),
            static_cast<std::uint8_t>(60 + pulse * 18)
        ));
    } else if (hovered) {
        float pulse = 0.5f + 0.5f * std::sin(t * 4.f + idx * 0.4f);
        card.setFillColor(sf::Color(
            static_cast<std::uint8_t>(58 + pulse * 14),
            static_cast<std::uint8_t>(58 + pulse * 14),
            static_cast<std::uint8_t>(88 + pulse * 20)
        ));
        card.setOutlineColor(sf::Color(
            static_cast<std::uint8_t>(160 + pulse * 18),
            static_cast<std::uint8_t>(160 + pulse * 18),
            static_cast<std::uint8_t>(200 + pulse * 18)
        ));
    } else {
        card.setFillColor(sf::Color(40,40,60));
        card.setOutlineColor(sf::Color(100,100,140));
    }

    if (alive) {
        const int hoverBoost = static_cast<int>(10.f * cardHoverT);
        const sf::Color base = card.getFillColor();
        card.setFillColor(sf::Color(
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.r) + hoverBoost)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.g) + hoverBoost)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.b) + hoverBoost)),
            base.a));
    }
    window.draw(card);

    if (!alive) {
        // Subtle dark veil + cross marker for immediate dead-state readability.
        sf::RectangleShape veil({cardW, cardH});
        veil.setPosition({cardX, cardY});
        veil.setFillColor(sf::Color(0, 0, 0, 70));
        window.draw(veil);

        sf::RectangleShape crossA({cardW + 20.f, 2.f});
        crossA.setPosition({cardX - 10.f, cardY + 10.f});
        crossA.setRotation(sf::degrees(23.f));
        crossA.setFillColor(sf::Color(239, 68, 68, 105));
        window.draw(crossA);

        sf::RectangleShape crossB({cardW + 20.f, 2.f});
        crossB.setPosition({cardX + cardW - 10.f, cardY + 10.f});
        crossB.setRotation(sf::degrees(157.f));
        crossB.setFillColor(sf::Color(239, 68, 68, 95));
        window.draw(crossB);
    }
    drawDebugBounds(sf::FloatRect({cardX, cardY}, {cardW, cardH}), sf::Color(255, 80, 220), 1.5f);

    if (selected && alive) {
        float glowPulse = 0.5f + 0.5f * std::sin(t * 6.f + idx);
        sf::RectangleShape glow({cardW + 8.f, cardH + 8.f});
        glow.setPosition({cardX - 4.f, cardY - 4.f});
        glow.setFillColor(sf::Color(0,0,0,0));
        glow.setOutlineThickness(3.f);
        glow.setOutlineColor(sf::Color(255, 120, 120,
            static_cast<std::uint8_t>(110 + glowPulse * 90)));
        window.draw(glow);
    }

    // Crewmate avatar
    drawCrewmate(cardX + 22.f, cardY + cardH * 0.5f, 0.7f, kColors[idx % 10], !alive);

    // Name
    if (fontLoaded) {
        sf::Text nm(font, p.getName(), 15);
        nm.setFillColor(alive ? sf::Color::White : sf::Color(224, 92, 92));
        auto b = nm.getLocalBounds();
        nm.setPosition({cardX + cardW * 0.55f - b.size.x/2.f - b.position.x, cardY + 28.f});
        snapTextToPixel(nm);
        window.draw(nm);
        drawDebugTextBounds(nm, sf::Color(110, 255, 120));

        // Status or silenced badge
        if (p.isSilenced() && alive) {
            sf::Text st(font, "SILENCED", 9);
            st.setFillColor(sf::Color(200,100,255));
            auto sb = st.getLocalBounds();
            st.setPosition({cardX + cardW - kUiContainerPaddingPx - sb.size.x - sb.position.x,
                            cardY + cardH - (kUiContainerPaddingPx + sb.size.y) - sb.position.y});
            window.draw(st);
            drawDebugTextBounds(st, sf::Color(110, 255, 120));
        } else {
            sf::Text st(font, alive ? "ALIVE" : "DEAD", 10);
            st.setFillColor(alive ? sf::Color(50,220,50) : sf::Color(239,68,68));
            auto sb = st.getLocalBounds();
            st.setPosition({cardX + cardW - kUiContainerPaddingPx - sb.size.x - sb.position.x,
                            cardY + cardH - (kUiContainerPaddingPx + sb.size.y) - sb.position.y});
            window.draw(st);
            drawDebugTextBounds(st, sf::Color(110, 255, 120));
        }

        // Vote badge
        if (showVotes && voteCount > 0 && alive) {
            std::string vstr = std::to_string(voteCount) + " vote" + (voteCount!=1?"s":"");
            sf::Text vt(font, vstr, 12);
            vt.setFillColor(sf::Color(255,200,0));
            auto vb = vt.getLocalBounds();
            vt.setPosition({cardX + cardW * 0.55f - vb.size.x/2.f - vb.position.x, cardY + cardH * 0.62f});
            snapTextToPixel(vt);
            window.draw(vt);
            drawDebugTextBounds(vt, sf::Color(110, 255, 120));
        }
    }
}

void Game::drawBackdrop(sf::Color base, sf::Color accent)
{
    float t = uiClock.getElapsedTime().asSeconds();

    // Force a consistent deep dark-blue foundation for the mystery theme.
    (void)base;
    sf::RectangleShape bg({1280.f, 720.f});
    bg.setFillColor(kThemeBackground);
    window.draw(bg);

    // Low-opacity depth wash to avoid a flat background.
    sf::RectangleShape depthWash({1280.f, 720.f});
    depthWash.setFillColor(withAlpha(kThemeCard, 24));
    window.draw(depthWash);

    sf::RectangleShape topBand({1280.f, 160.f});
    topBand.setFillColor(withAlpha(accent, 60));
    window.draw(topBand);

    sf::CircleShape glow1(220.f);
    glow1.setPosition({-70.f + std::sin(t * 0.35f) * 16.f, -120.f});
    glow1.setFillColor(withAlpha(kThemeAccentBlue, 22));
    window.draw(glow1);

    sf::CircleShape glow2(180.f);
    glow2.setPosition({960.f - std::sin(t * 0.42f) * 20.f, 520.f});
    glow2.setFillColor(withAlpha(kThemeDangerRed, 15));
    window.draw(glow2);

    for (int i = 0; i < 6; ++i) {
        sf::RectangleShape stripe({1280.f, 1.f});
        stripe.setPosition({0.f, 110.f + i * 96.f + std::sin(t * 0.6f + i) * 2.f});
        stripe.setFillColor(withAlpha(kThemeTextPrimary, 5));
        window.draw(stripe);
    }

    // Subtle vignette: progressively darken edges toward the screen borders.
    for (int i = 0; i < 7; ++i) {
        const float inset = i * 16.f;
        const float strip = 16.f;
        const std::uint8_t a = static_cast<std::uint8_t>(std::max(0, 38 - i * 5));

        sf::RectangleShape top({1280.f - inset * 2.f, strip});
        top.setPosition({inset, inset});
        top.setFillColor(sf::Color(0, 0, 0, a));
        window.draw(top);

        sf::RectangleShape bottom({1280.f - inset * 2.f, strip});
        bottom.setPosition({inset, 720.f - inset - strip});
        bottom.setFillColor(sf::Color(0, 0, 0, a));
        window.draw(bottom);

        sf::RectangleShape left({strip, 720.f - inset * 2.f});
        left.setPosition({inset, inset});
        left.setFillColor(sf::Color(0, 0, 0, a));
        window.draw(left);

        sf::RectangleShape right({strip, 720.f - inset * 2.f});
        right.setPosition({1280.f - inset - strip, inset});
        right.setFillColor(sf::Color(0, 0, 0, a));
        window.draw(right);
    }
}

void Game::drawPanel(const sf::FloatRect& r, sf::Color fill, sf::Color outline)
{
    const sf::FloatRect safeR = clampToSafeArea(r);

    sf::RectangleShape shadow({safeR.size.x + 10.f, safeR.size.y + 10.f});
    shadow.setPosition({safeR.position.x + 5.f, safeR.position.y + 6.f});
    shadow.setFillColor(sf::Color(0, 0, 0, 90));
    window.draw(shadow);

    sf::RectangleShape panel({safeR.size.x, safeR.size.y});
    panel.setPosition({safeR.position.x, safeR.position.y});
    panel.setFillColor(fill);
    panel.setOutlineColor(outline);
    panel.setOutlineThickness(1.f);
    window.draw(panel);
    drawDebugBounds(safeR, sf::Color(80, 190, 255), 1.5f);

    sf::RectangleShape accent({4.f, safeR.size.y});
    accent.setPosition({safeR.position.x, safeR.position.y});
    accent.setFillColor(withAlpha(kThemeAccentBlue, 120));
    window.draw(accent);
}

void Game::drawSectionHeader(const std::string& title, const std::string& subtitle,
                             float y, sf::Color accent)
{
    if (!fontLoaded) return;

    constexpr float kTitleToLineGap = 8.f;
    constexpr float kLineToSubtitleGap = 14.f;

    const float centerX = window.getView().getSize().x * 0.5f;

    sf::Text titleText(font, title, 30);
    titleText.setFillColor(kThemeTextPrimary);
    auto titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x * 0.5f, titleBounds.position.y});
    titleText.setPosition({centerX, y});
    snapTextToPixel(titleText);
    window.draw(titleText);
    drawDebugTextBounds(titleText, sf::Color(110, 255, 120));

    const auto titleGlobal = titleText.getGlobalBounds();
    const float titleBottomY = titleGlobal.position.y + titleGlobal.size.y;
    const float lineY = std::round(titleBottomY + kTitleToLineGap);
    const float subtitleY = std::round(lineY + kLineToSubtitleGap);

    float timeSec = uiClock.getElapsedTime().asSeconds();

    sf::RectangleShape leftLine({150.f, 2.f});
    leftLine.setPosition({centerX - 188.f, lineY});
    leftLine.setFillColor(accent);
    window.draw(leftLine);

    sf::RectangleShape rightLine({150.f, 2.f});
    rightLine.setPosition({centerX + 38.f, lineY});
    rightLine.setFillColor(accent);
    window.draw(rightLine);

    sf::RectangleShape pulse({36.f, 4.f});
    pulse.setPosition({centerX - 18.f + std::sin(timeSec * 2.f) * 18.f, lineY - 1.f});
    pulse.setFillColor(sf::Color(accent.r, accent.g, accent.b, 180));
    window.draw(pulse);

    if (!subtitle.empty()) {
        sf::Text s(font, subtitle, 15);
        s.setFillColor(kThemeTextSecondary);
        const float subMaxW = window.getView().getSize().x * 0.74f;
        applyWrappedText(s, font, 15, subtitle, subMaxW);
        auto subBounds = s.getLocalBounds();
        s.setOrigin({subBounds.position.x + subBounds.size.x * 0.5f, subBounds.position.y});
        s.setPosition({centerX, subtitleY});
        snapTextToPixel(s);
        window.draw(s);
        drawDebugTextBounds(s, sf::Color(110, 255, 120));
    }
}

void Game::drawBadge(const sf::FloatRect& r, const std::string& label,
                     sf::Color fill, sf::Color outline)
{
    sf::RectangleShape badge({r.size.x, r.size.y});
    badge.setPosition({r.position.x, r.position.y});
    badge.setFillColor(fill);
    badge.setOutlineColor(outline);
    badge.setOutlineThickness(1.f);
    window.draw(badge);

    if (!fontLoaded) return;

    sf::Text t(font, label, 14);
    t.setFillColor(kThemeTextPrimary);
    auto b = t.getLocalBounds();
    t.setPosition({r.position.x + r.size.x/2.f - b.size.x/2.f - b.position.x, r.position.y + r.size.y/2.f - b.size.y/2.f - b.position.y - 1.f});
    snapTextToPixel(t);
    window.draw(t);
    drawDebugTextBounds(t, sf::Color(110, 255, 120));
}

void Game::drawTopHud(const std::string& phaseTitle, sf::Color accent)
{
    sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);
    float safeX = safe.position.x;
    float safeY = safe.position.y;
    drawPanel(sf::FloatRect({safeX, safeY}, {safe.size.x, 52.f}),
              withAlpha(kThemeCard, 220), withAlpha(accent, 140));

    static std::string lastPhaseTitle;
    static float phaseFadeStart = 0.f;
    const float now = uiClock.getElapsedTime().asSeconds();
    if (lastPhaseTitle != phaseTitle) {
        lastPhaseTitle = phaseTitle;
        phaseFadeStart = now;
    }
    const float fadeT = std::clamp((now - phaseFadeStart) / 0.32f, 0.f, 1.f);

    auto brighten = [](sf::Color c, int bump) {
        return sf::Color(
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(c.r) + bump)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(c.g) + bump)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(c.b) + bump)),
            c.a);
    };

    sf::Color phaseCol = brighten(accent, 36);
    phaseCol.a = static_cast<std::uint8_t>(110 + 145 * fadeT);

    if (fontLoaded) {
        sf::Text phaseText(font, spacedLabel(phaseTitle), 28);
        phaseText.setFillColor(phaseCol);
        auto pb = phaseText.getLocalBounds();
        phaseText.setOrigin({pb.position.x + pb.size.x * 0.5f, pb.position.y});
        phaseText.setPosition({safeX + safe.size.x * 0.5f, safeY + 2.f + (1.f - fadeT) * 8.f});
        snapTextToPixel(phaseText);
        window.draw(phaseText);

        sf::RectangleShape phaseLine({220.f * fadeT, 2.f});
        phaseLine.setOrigin({110.f * fadeT, 0.f});
        phaseLine.setPosition({safeX + safe.size.x * 0.5f, safeY + 34.f});
        phaseLine.setFillColor(withAlpha(kThemeAccentBlue, static_cast<std::uint8_t>(80 + 80 * fadeT)));
        window.draw(phaseLine);
    }

    int alive = 0;
    int dead = 0;
    for (const auto& p : players) {
        if (p.getIsAlive()) alive++;
        else dead++;
    }

    std::string roundText = "Round " + std::to_string(roundNumber);
    std::string statusText = "Alive " + std::to_string(alive) + "  Dead " + std::to_string(dead);

    if (fontLoaded) {
        sf::Text rt(font, roundText, 16);
        rt.setFillColor(kThemeTextPrimary);
        placeTextTopLeft(rt, safeX + 16.f, safeY + 30.f);
        window.draw(rt);
        drawDebugTextBounds(rt, sf::Color(110, 255, 120));

        sf::Text st(font, statusText, 15);
        st.setFillColor(kThemeTextSecondary);
        placeTextTopLeft(st, safeX + 140.f, safeY + 31.f);
        window.draw(st);
        drawDebugTextBounds(st, sf::Color(110, 255, 120));

        if (!players.empty() && humanIdx < (int)players.size()) {
            std::string roleText = "Your role: " + players[humanIdx].getRoleName();
            sf::Text hr(font, roleText, 15);
            hr.setFillColor(kThemeTextSecondary);
            auto b = hr.getLocalBounds();
            placeTextTopLeft(hr, safeX + safe.size.x - 16.f - b.size.x, safeY + 31.f);
            window.draw(hr);
            drawDebugTextBounds(hr, sf::Color(110, 255, 120));
        }
    }
}

void Game::drawButton(const sf::FloatRect& r, const std::string& label,
                       sf::Color fill, sf::Color txtCol)
{
    constexpr float kButtonShadowOffsetX = 4.f;
    constexpr float kButtonShadowOffsetY = 5.f;
    constexpr float kButtonTextYOffset = -1.f;

    sf::FloatRect safeR = clampToSafeArea(r);
    bool hovered = btnHovered(safeR);
    bool pressedNow = hovered && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

    // Keep a tiny animation state per button for smooth, subtle transitions.
    auto mkAnimKey = [&](const sf::FloatRect& rr, const std::string& lbl) {
        std::string k = lbl + ":" +
                        std::to_string(static_cast<int>(rr.position.x * 10.f)) + ":" +
                        std::to_string(static_cast<int>(rr.position.y * 10.f)) + ":" +
                        std::to_string(static_cast<int>(rr.size.x * 10.f)) + ":" +
                        std::to_string(static_cast<int>(rr.size.y * 10.f));
        return std::hash<std::string>{}(k);
    };
    const std::size_t animKey = mkAnimKey(safeR, label);
    static std::unordered_map<std::size_t, float> hoverAnim;
    static std::unordered_map<std::size_t, float> pressAnim;

    float& hoverT = hoverAnim[animKey];
    float& pressT = pressAnim[animKey];
    hoverT += ((hovered ? 1.f : 0.f) - hoverT) * 0.20f;
    pressT += ((pressedNow ? 1.f : 0.f) - pressT) * 0.28f;

    float hoverLift = -1.8f * hoverT + 1.2f * pressT;

    // Hover brightness is eased instead of binary.
    const int hoverBoost = static_cast<int>(14.f * hoverT);
    sf::Color bg = sf::Color(
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(fill.r) + hoverBoost)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(fill.g) + hoverBoost)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(fill.b) + hoverBoost)),
        fill.a);

    sf::RectangleShape shadow({safeR.size.x + 8.f, safeR.size.y + 8.f});
    shadow.setPosition({safeR.position.x + kButtonShadowOffsetX, safeR.position.y + kButtonShadowOffsetY + hoverLift});
    shadow.setFillColor(sf::Color(0, 0, 0, 85));
    window.draw(shadow);

    sf::Vector2f btnSize(safeR.size.x, safeR.size.y);
    const float pressScaleX = 1.f - 0.014f * pressT;
    const float pressScaleY = 1.f - 0.026f * pressT;
    btnSize.x = std::max(20.f, safeR.size.x * pressScaleX);
    btnSize.y = std::max(20.f, safeR.size.y * pressScaleY);

    sf::RectangleShape btn({btnSize.x, btnSize.y});
    btn.setPosition({safeR.position.x, safeR.position.y + hoverLift});
    btn.setFillColor(bg);
    const std::uint8_t outlineA = static_cast<std::uint8_t>(80 + std::min(120.f, 120.f * hoverT));
    btn.setOutlineColor(withAlpha(kThemeAccentBlue, outlineA));
    btn.setOutlineThickness(1.f);
    window.draw(btn);
    drawDebugBounds(sf::FloatRect({safeR.position.x, safeR.position.y + hoverLift}, {btnSize.x, btnSize.y}), sf::Color(255, 210, 80), 1.2f);

    sf::RectangleShape accent({4.f, btnSize.y});
    accent.setPosition({safeR.position.x, safeR.position.y + hoverLift});
    const std::uint8_t accentA = static_cast<std::uint8_t>(110 + std::min(90.f, 90.f * hoverT));
    accent.setFillColor(withAlpha(kThemeAccentBlue, accentA));
    window.draw(accent);

    if (fontLoaded) {
        sf::Text t(font, label, 19);
        t.setFillColor(txtCol);
        auto b = t.getLocalBounds();
        t.setPosition({safeR.position.x + btnSize.x/2.f - b.size.x/2.f - b.position.x, safeR.position.y + hoverLift + btnSize.y/2.f - b.size.y/2.f - b.position.y + kButtonTextYOffset});
        snapTextToPixel(t);
        window.draw(t);
        drawDebugTextBounds(t, sf::Color(110, 255, 120));
    }
}

bool Game::btnClicked(const sf::FloatRect& r)
{
    sf::FloatRect safeR = clampToSafeArea(r);
    bool clicked = mouseClicked && safeR.contains(clickPos);
    if (clicked) playSfx(sfxClick.get(), 35.f, true);
    return clicked;
}

bool Game::btnHovered(const sf::FloatRect& r) const
{
    sf::FloatRect safeR = clampToSafeArea(r);
    return safeR.contains(mousePos);
}

void Game::drawCentered(const std::string& s, unsigned sz, sf::Color c, float y)
{
    if (!fontLoaded) return;
    sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);
    sf::Text t(font, s, sz);
    t.setFillColor(c);
    auto b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
    float clampedY = std::clamp(y, safe.position.y, safe.position.y + safe.size.y - std::max(1.f, b.size.y));
    t.setPosition({safe.position.x + safe.size.x * 0.5f, clampedY});
    snapTextToPixel(t);
    window.draw(t);
    drawDebugTextBounds(t, sf::Color(110, 255, 120));
}

void Game::drawProgressBar(float progress, float yPos, float h)
{
    progress = std::clamp(progress, 0.f, 1.f);
    float t = uiClock.getElapsedTime().asSeconds();
    sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);

    float safeY = std::max(safe.position.y, yPos);

    sf::RectangleShape bg({safe.size.x, h});
    bg.setPosition({safe.position.x, safeY});
    bg.setFillColor(kThemeCard);
    window.draw(bg);

    sf::Color col = progress > 0.5f ? withAlpha(kThemeAccentBlue, 210)
                  : progress > 0.25f ? withAlpha(kThemeTextSecondary, 210)
                  :                    withAlpha(kThemeDangerRed, 220);
    sf::RectangleShape bar({safe.size.x * progress, h});
    bar.setPosition({safe.position.x, safeY});
    bar.setFillColor(col);
    window.draw(bar);

    if (progress > 0.02f) {
        float sweepW = 110.f;
        float sweepX = safe.position.x + std::fmod(t * 180.f, std::max(1.f, safe.size.x * progress + sweepW)) - sweepW;
        sf::RectangleShape sweep({sweepW, h});
        sweep.setPosition({sweepX, safeY});
        sweep.setFillColor(sf::Color(255, 255, 255, 40));
        window.draw(sweep);
    }

    sf::RectangleShape edge({safe.size.x, 1.f});
    edge.setPosition({safe.position.x, safeY + h});
    edge.setFillColor(sf::Color(255, 255, 255, 35));
    window.draw(edge);
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderMainMenu
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderMainMenu()
{
    drawBackdrop(kThemeBackground, withAlpha(kThemeAccentBlue, 36));

    if (!fontLoaded) return;

    const float centerX = window.getView().getSize().x * 0.5f;

    drawCentered("MAFIA", 84, kThemeDangerRed, 140.f);
    drawCentered("The Hidden Enemy", 26, kThemeTextPrimary, 228.f);

    const float buttonW = 280.f;
    const float playW = 336.f;
    const float buttonH = 54.f;
    const float playH = 64.f;
    const float buttonX = centerX - buttonW * 0.5f;
    const float playX = centerX - playW * 0.5f;

    sf::FloatRect playBtn({playX, 332.f}, {playW, playH});
    sf::FloatRect instructionsBtn({buttonX, 412.f}, {buttonW, buttonH});
    sf::FloatRect settingsBtn({buttonX, 482.f}, {buttonW, buttonH});
    sf::FloatRect exitBtn({buttonX, 552.f}, {buttonW, buttonH});

    drawButton(playBtn, "PLAY", kThemeAccentBlue);
    drawButton(instructionsBtn, "INSTRUCTIONS", withAlpha(kThemeCard, 255));
    drawButton(settingsBtn, "SETTINGS", withAlpha(kThemeCard, 255));
    drawButton(exitBtn, "EXIT", kThemeDangerRed);

    if (btnClicked(playBtn)) {
        transitionTo(Phase::LOBBY);
    }
    if (btnClicked(instructionsBtn)) {
        transitionTo(Phase::INSTRUCTIONS);
    }
    if (btnClicked(settingsBtn)) {
        transitionTo(Phase::SETTINGS);
    }
    if (btnClicked(exitBtn)) {
        window.close();
    }
}

void Game::renderInstructions()
{
    drawBackdrop(sf::Color(12, 16, 24), sf::Color(225, 170, 90, 34));
    sf::FloatRect panelRect = uiRect(0.05f, 0.07f, 0.90f, 0.84f);
    drawPanel(panelRect, sf::Color(16, 22, 30, 238), sf::Color(205, 175, 120, 130));

    if (!fontLoaded) return;

    drawSectionHeader("INSTRUCTIONS", "Quick scan guide", panelRect.position.y + 18.f, sf::Color(235, 205, 145));

    const float contentTop = panelRect.position.y + 98.f;
    const float contentBottom = panelRect.position.y + panelRect.size.y - 86.f;
    const float contentHeight = contentBottom - contentTop;
    const sf::FloatRect contentRect({panelRect.position.x + 30.f, contentTop}, {panelRect.size.x - 72.f, contentHeight});
    const float columnGap = kUiSectionSpacingPx;
    const float columnWidth = (contentRect.size.x - columnGap) * 0.5f;
    const float columnBodyWidth = columnWidth - 18.f;
    const float lineGap = 4.f;
    const float sectionGap = kUiSectionSpacingPx;

    struct Section {
        const char* heading;
        std::vector<std::string> lines;
    };

    const std::vector<Section> leftSections = {
        {"OBJECTIVE", {
            "Villagers win by eliminating all mafia members.",
            "Mafia win when they equal or outnumber non-mafia players.",
            "Joker wins by getting voted out during the day."
        }},
        {"ROUND FLOW", {
            "Night: Mafia attack, Doctor saves, Detective investigates, Silencer mutes.",
            "Day: The result is revealed to everyone.",
            "Discussion: Players debate, accuse, and defend.",
            "Voting: The highest-voted player is eliminated."
        }},
        {"CONTROLS", {
            "Mouse: Click buttons and choose targets.",
            "Esc: Return Home here, or open the quick menu in a match.",
            "Q: Open the quick menu during gameplay.",
            "Tab: Toggle the player info panel in match.",
            "M: Mute or unmute all audio.",
            "[ and ]: Music volume. - and =: SFX volume."
        }}
    };

    const std::vector<Section> rightSections = {
        {"ROLES", {
            "GodFather: Mafia leader. Appears innocent to detective checks.",
            "Mafia: Helps choose the night kill.",
            "Silencer: Stops one target from speaking next day.",
            "Doctor: Protects one player from elimination.",
            "Detective: Checks if a player is mafia aligned.",
            "Villager: No night action. Use logic and votes."
        }},
        {"QUICK TIPS", {
            "Read the room before voting.",
            "Track who benefits from each elimination.",
            "Keep claims short and consistent.",
            "If content grows too long, scroll down."
        }}
    };

    auto measureTextHeight = [&](const std::string& text, unsigned size, float maxWidth) {
        float total = 0.f;
        std::string wrapped = wrapTextToWidth(font, size, text, maxWidth);
        for (const auto& line : splitByNewline(wrapped)) {
            sf::Text measure(font, line.empty() ? " " : line, size);
            total += measure.getLocalBounds().size.y + lineGap;
        }
        return total;
    };

    auto measureSectionHeight = [&](const Section& section) {
        float total = 0.f;
        sf::Text heading(font, section.heading, 22);
        total += heading.getLocalBounds().size.y + 10.f;
        for (const auto& line : section.lines) {
            total += measureTextHeight(line, 15, columnBodyWidth);
        }
        return total + sectionGap;
    };

    float leftHeight = 0.f;
    for (const auto& section : leftSections) leftHeight += measureSectionHeight(section);
    float rightHeight = 0.f;
    for (const auto& section : rightSections) rightHeight += measureSectionHeight(section);
    float totalContentHeight = std::max(leftHeight, rightHeight);
    float maxScroll = std::max(0.f, totalContentHeight - contentRect.size.y);
    instructionsScroll = std::clamp(instructionsScroll, 0.f, maxScroll);

    auto drawSection = [&](float x, float y, const Section& section) {
        sf::Text heading(font, section.heading, 22);
        heading.setFillColor(sf::Color(235, 205, 145));
        heading.setPosition({std::round(x), std::round(y)});
        if (y + heading.getLocalBounds().size.y >= contentRect.position.y &&
            y <= contentRect.position.y + contentRect.size.y) {
            window.draw(heading);
        }
        y += heading.getLocalBounds().size.y + 10.f;

        for (const auto& line : section.lines) {
            std::string wrapped = wrapTextToWidth(font, 15, line, columnBodyWidth);
            for (const auto& wrappedLine : splitByNewline(wrapped)) {
                sf::Text body(font, wrappedLine.empty() ? " " : wrappedLine, 15);
                body.setFillColor(sf::Color(220, 230, 240));
                body.setPosition({std::round(x), std::round(y)});
                if (y + body.getLocalBounds().size.y >= contentRect.position.y &&
                    y <= contentRect.position.y + contentRect.size.y) {
                    window.draw(body);
                }
                y += body.getLocalBounds().size.y + lineGap;
            }
            y += 3.f;
        }
        return y + sectionGap;
    };

    const float leftX = contentRect.position.x;
    const float rightX = leftX + columnWidth + columnGap;
    float leftY = contentRect.position.y - instructionsScroll;
    float rightY = contentRect.position.y - instructionsScroll;
    for (const auto& section : leftSections) {
        leftY = drawSection(leftX, leftY, section);
    }
    for (const auto& section : rightSections) {
        rightY = drawSection(rightX, rightY, section);
    }

    if (maxScroll > 0.f) {
        const float trackX = contentRect.position.x + contentRect.size.x + 10.f;
        const float trackY = contentRect.position.y;
        const float trackH = contentRect.size.y;
        sf::RectangleShape track({8.f, trackH});
        track.setPosition({trackX, trackY});
        track.setFillColor(sf::Color(255, 255, 255, 18));
        window.draw(track);

        float thumbH = std::max(34.f, trackH * (contentRect.size.y / totalContentHeight));
        float thumbY = trackY + (instructionsScroll / maxScroll) * (trackH - thumbH);
        sf::RectangleShape thumb({8.f, thumbH});
        thumb.setPosition({trackX, thumbY});
        thumb.setFillColor(sf::Color(235, 205, 145, 170));
        window.draw(thumb);
    }

    sf::FloatRect backBtn = uiRect(0.38f, 0.84f, 0.24f, 0.064f);
    drawButton(backBtn, "BACK TO HOME", sf::Color(75, 75, 105));
    if (btnClicked(backBtn)) {
        transitionTo(Phase::MAIN_MENU);
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderSettings
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderSettings()
{
    drawBackdrop(sf::Color(12, 14, 20), sf::Color(70, 130, 210, 36));

    sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);
    sf::FloatRect panelRect({safe.position.x + 30.f, safe.position.y + 30.f}, {safe.size.x - 60.f, safe.size.y - 70.f});
    drawPanel(panelRect, sf::Color(14, 18, 28, 238), sf::Color(90, 120, 180, 130));

    if (!fontLoaded) return;

    drawSectionHeader("SETTINGS", "Tune pacing, AI intensity, and audio mix", 92.f, sf::Color(90, 150, 230));

    auto stepButton = [&](const sf::FloatRect& r, const std::string& label, sf::Color fill) {
        drawButton(r, label, fill);
    };

    auto rowHoverColor = [](bool hovered) {
        return hovered ? sf::Color(70, 105, 155, 65) : sf::Color(0, 0, 0, 0);
    };

    auto drawRowShell = [&](const sf::FloatRect& rowRect, bool hovered) {
        sf::RectangleShape row(rowRect.size);
        row.setPosition(rowRect.position);
        row.setFillColor(rowHoverColor(hovered));
        row.setOutlineColor(hovered ? sf::Color(120, 165, 235, 130) : sf::Color(60, 80, 120, 55));
        row.setOutlineThickness(1.f);
        window.draw(row);
    };

    auto drawGroupPanel = [&](const sf::FloatRect& rect, const std::string& title, const std::string& subtitle, sf::Color accent) {
        drawPanel(rect, sf::Color(18, 24, 36, 214), withAlpha(accent, 110));
        const sf::FloatRect inner = insetRect(rect);
        sf::Text hdr(font, title, 22);
        hdr.setFillColor(sf::Color(235, 205, 145));
        hdr.setPosition({inner.position.x, inner.position.y});
        window.draw(hdr);
        sf::Text sub(font, subtitle, 14);
        sub.setFillColor(sf::Color(185, 195, 215));
        applyWrappedText(sub, font, 14, subtitle, inner.size.x);
        sub.setPosition({inner.position.x, inner.position.y + 28.f});
        window.draw(sub);
    };

    bool settingsChanged = false;
    static float previewMenuAmbUntil = 0.f;
    static float previewGameAmbUntil = 0.f;

    sf::FloatRect gameplayPanel({panelRect.position.x + 18.f, panelRect.position.y + 92.f}, {panelRect.size.x - 36.f, 300.f});
    sf::FloatRect audioPanel({panelRect.position.x + 18.f, panelRect.position.y + 406.f}, {panelRect.size.x - 36.f, 256.f});
    sf::FloatRect previewPanel({panelRect.position.x + panelRect.size.x - 292.f, panelRect.position.y + 92.f}, {274.f, 570.f});
    drawGroupPanel(gameplayPanel, "Gameplay", "Match pacing and AI behavior", sf::Color(90, 150, 230));
    drawGroupPanel(audioPanel, "Audio", "Volumes and playback preview", sf::Color(140, 100, 220));
    drawGroupPanel(previewPanel, "Preview", "Tap to audition UI sounds", sf::Color(90, 190, 210));

    const sf::FloatRect gameplayInner = insetRect(gameplayPanel);
    const sf::FloatRect audioInner = insetRect(audioPanel);
    const float formValuePct = 0.56f;
    const float labelX = gameplayInner.position.x;
    const float valueX = gameplayInner.position.x + gameplayInner.size.x * formValuePct;
    const float audioLabelX = audioInner.position.x;
    const float audioValueX = audioInner.position.x + audioInner.size.x * formValuePct;
    const float rowH = 54.f;
    const float gameplayControlsTop = gameplayInner.position.y + 56.f;
    const float audioControlsTop = audioInner.position.y + 56.f;
    VStack gameplayStack(gameplayInner.position.x, gameplayControlsTop, gameplayInner.size.x, kUiSectionSpacingPx);
    VStack audioStack(audioInner.position.x, audioControlsTop, audioInner.size.x, kUiSectionSpacingPx);

    auto drawValueRow = [&](const sf::FloatRect& slot, const std::string& label, std::string value,
                            const sf::FloatRect& minusBtn, const sf::FloatRect& plusBtn) {
        const float y = slot.position.y;
        sf::FloatRect rowRect({gameplayInner.position.x - 2.f, y - 8.f}, {gameplayInner.size.x + 4.f, rowH});
        drawRowShell(rowRect, btnHovered(rowRect));

        sf::Text lbl(font, label, 20);
        lbl.setFillColor(sf::Color(220, 225, 235));
        fitTextToWidth(lbl, valueX - labelX - 14.f, 14);
        lbl.setPosition({labelX, y});
        window.draw(lbl);

        sf::RectangleShape valBox({210.f, 42.f});
        valBox.setPosition({valueX, y - 4.f});
        valBox.setFillColor(sf::Color(32, 32, 48));
        valBox.setOutlineColor(sf::Color(90, 90, 120));
        valBox.setOutlineThickness(1.f);
        window.draw(valBox);

        sf::Text val(font, value, 18);
        val.setFillColor(sf::Color::White);
        auto vb = val.getLocalBounds();
        val.setPosition({valueX + 105.f - vb.size.x / 2.f - vb.position.x, y + 5.f});
        window.draw(val);

        stepButton(minusBtn, "-", sf::Color(70, 70, 100));
        stepButton(plusBtn, "+", sf::Color(70, 70, 100));
    };

    const float stepBtnW = 44.f;
    const float stepBtnGap = 10.f;
    const float stepBtnX1 = gameplayInner.position.x + gameplayInner.size.x - (stepBtnW * 2.f + stepBtnGap);
    const float stepBtnX2 = stepBtnX1 + stepBtnW + stepBtnGap;

    sf::FloatRect nightSlot = gameplayStack.next(rowH);
    sf::FloatRect nMinus({stepBtnX1, nightSlot.position.y - 4.f}, {stepBtnW, 42.f});
    sf::FloatRect nPlus ({stepBtnX2, nightSlot.position.y - 4.f}, {stepBtnW, 42.f});
    drawValueRow(nightSlot, "Night Duration", std::to_string((int)nightDurationSec) + " sec", nMinus, nPlus);

    sf::FloatRect discussSlot = gameplayStack.next(rowH);
    sf::FloatRect dMinus({stepBtnX1, discussSlot.position.y - 4.f}, {stepBtnW, 42.f});
    sf::FloatRect dPlus ({stepBtnX2, discussSlot.position.y - 4.f}, {stepBtnW, 42.f});
    drawValueRow(discussSlot, "Discussion Duration", std::to_string((int)discussionDurationSec) + " sec", dMinus, dPlus);

    sf::FloatRect biasSlot = gameplayStack.next(rowH);
    sf::FloatRect aMinus({stepBtnX1, biasSlot.position.y - 4.f}, {stepBtnW, 42.f});
    sf::FloatRect aPlus ({stepBtnX2, biasSlot.position.y - 4.f}, {stepBtnW, 42.f});
    drawValueRow(biasSlot, "AI Suspicion Bias", (aiAccusationBias > 0 ? "+" : "") + std::to_string(aiAccusationBias), aMinus, aPlus);

    sf::Text paceLbl(font, "AI Chat Pace", 20);
    paceLbl.setFillColor(sf::Color(220, 225, 235));
    sf::FloatRect paceSlot = gameplayStack.next(rowH);
    paceLbl.setPosition({labelX, paceSlot.position.y});
    window.draw(paceLbl);

    sf::FloatRect paceSlow({valueX, paceSlot.position.y - 4.f}, {94.f, 42.f});
    sf::FloatRect paceNormal({valueX + 102.f, paceSlot.position.y - 4.f}, {104.f, 42.f});
    sf::FloatRect paceFast({valueX + 214.f, paceSlot.position.y - 4.f}, {84.f, 42.f});

    bool slow = aiChatMinDelaySec >= 5.f;
    bool fast = aiChatMaxDelaySec <= 4.f;
    bool normal = !slow && !fast;

    drawButton(paceSlow, "SLOW", slow ? sf::Color(40, 95, 120) : sf::Color(55, 55, 80));
    drawButton(paceNormal, "NORMAL", normal ? sf::Color(40, 95, 120) : sf::Color(55, 55, 80));
    drawButton(paceFast, "FAST", fast ? sf::Color(40, 95, 120) : sf::Color(55, 55, 80));

    if (btnClicked(nMinus)) { nightDurationSec = std::max(15.f, nightDurationSec - 5.f); settingsChanged = true; }
    if (btnClicked(nPlus))  { nightDurationSec = std::min(60.f, nightDurationSec + 5.f); settingsChanged = true; }

    if (btnClicked(dMinus)) { discussionDurationSec = std::max(30.f, discussionDurationSec - 10.f); settingsChanged = true; }
    if (btnClicked(dPlus))  { discussionDurationSec = std::min(120.f, discussionDurationSec + 10.f); settingsChanged = true; }

    if (btnClicked(aMinus)) { aiAccusationBias = std::max(-20, aiAccusationBias - 5); settingsChanged = true; }
    if (btnClicked(aPlus))  { aiAccusationBias = std::min(20, aiAccusationBias + 5); settingsChanged = true; }

    if (btnClicked(paceSlow))   { aiChatMinDelaySec = 5.f; aiChatMaxDelaySec = 11.f; settingsChanged = true; }
    if (btnClicked(paceNormal)) { aiChatMinDelaySec = 3.f; aiChatMaxDelaySec = 8.f; settingsChanged = true; }
    if (btnClicked(paceFast))   { aiChatMinDelaySec = 1.5f; aiChatMaxDelaySec = 4.f; settingsChanged = true; }

    auto drawAudioRow = [&](const sf::FloatRect& slot, const std::string& label, float& value,
                            const sf::FloatRect& minusBtn, const sf::FloatRect& plusBtn) {
        const float y = slot.position.y;
        sf::FloatRect rowRect({audioInner.position.x - 2.f, y - 8.f}, {audioInner.size.x + 4.f, 44.f});
        drawRowShell(rowRect, btnHovered(rowRect));

        sf::Text lbl(font, label, 17);
        lbl.setFillColor(sf::Color(220, 225, 235));
        const float maxLabelW = audioValueX - audioLabelX - 14.f;
        fitTextToWidth(lbl, maxLabelW, 12);
        lbl.setPosition({audioLabelX, y});
        window.draw(lbl);

        sf::RectangleShape valBox({140.f, 36.f});
        valBox.setPosition({audioValueX, y - 3.f});
        valBox.setFillColor(sf::Color(28, 34, 50));
        valBox.setOutlineColor(sf::Color(90, 110, 140));
        valBox.setOutlineThickness(1.f);
        window.draw(valBox);

        sf::Text val(font, std::to_string((int)value) + "%", 16);
        val.setFillColor(sf::Color::White);
        auto vb = val.getLocalBounds();
        const float valCenterX = valBox.getPosition().x + valBox.getSize().x * 0.5f;
        val.setPosition({valCenterX - vb.size.x / 2.f - vb.position.x, y + 4.f});
        window.draw(val);

        drawButton(minusBtn, "-", sf::Color(70, 70, 100));
        drawButton(plusBtn, "+", sf::Color(70, 70, 100));

        if (btnClicked(minusBtn)) { value = std::max(0.f, value - 5.f); settingsChanged = true; }
        if (btnClicked(plusBtn))  { value = std::min(100.f, value + 5.f); settingsChanged = true; }
    };

    const float audioStepBtnW = 40.f;
    const float audioStepGap = 8.f;
    const float audioStepX1 = audioInner.position.x + audioInner.size.x - (audioStepBtnW * 2.f + audioStepGap);
    const float audioStepX2 = audioStepX1 + audioStepBtnW + audioStepGap;

    sf::FloatRect masterSlot = audioStack.next(44.f);
    drawAudioRow(masterSlot, "Master Volume", masterVolume,
                 sf::FloatRect({audioStepX1, masterSlot.position.y - 3.f}, {audioStepBtnW, 36.f}),
                 sf::FloatRect({audioStepX2, masterSlot.position.y - 3.f}, {audioStepBtnW, 36.f}));
    sf::FloatRect uiSlot = audioStack.next(44.f);
    drawAudioRow(uiSlot, "UI Volume", uiVolume,
                 sf::FloatRect({audioStepX1, uiSlot.position.y - 3.f}, {audioStepBtnW, 36.f}),
                 sf::FloatRect({audioStepX2, uiSlot.position.y - 3.f}, {audioStepBtnW, 36.f}));
    sf::FloatRect sfxSlot = audioStack.next(44.f);
    drawAudioRow(sfxSlot, "SFX Volume", sfxVolume,
                 sf::FloatRect({audioStepX1, sfxSlot.position.y - 3.f}, {audioStepBtnW, 36.f}),
                 sf::FloatRect({audioStepX2, sfxSlot.position.y - 3.f}, {audioStepBtnW, 36.f}));
    sf::FloatRect ambSlot = audioStack.next(44.f);
    drawAudioRow(ambSlot, "Ambient Volume", ambientVolume,
                 sf::FloatRect({audioStepX1, ambSlot.position.y - 3.f}, {audioStepBtnW, 36.f}),
                 sf::FloatRect({audioStepX2, ambSlot.position.y - 3.f}, {audioStepBtnW, 36.f}));

    // Audio preview panel
    sf::FloatRect audioPreviewPanel({previewPanel.position.x + 14.f, previewPanel.position.y + 54.f}, {previewPanel.size.x - 28.f, 390.f});
    drawPanel(audioPreviewPanel, sf::Color(20, 26, 40, 230), sf::Color(90, 130, 190, 140));
    const sf::FloatRect audioPreviewInner = insetRect(audioPreviewPanel);
    sf::Text apTitle(font, "AUDIO PREVIEW", 16);
    apTitle.setFillColor(sf::Color(180, 220, 255));
    apTitle.setPosition({audioPreviewInner.position.x, audioPreviewInner.position.y});
    window.draw(apTitle);

    auto previewBtn = [&](const sf::FloatRect& slot, const std::string& label, sf::Color col, sf::Sound* snd, float vol, bool uiBus) {
        sf::FloatRect r({audioPreviewInner.position.x, slot.position.y}, {audioPreviewInner.size.x, slot.size.y});
        drawButton(r, label, col);
        if (btnClicked(r)) {
            playSfx(snd, vol, uiBus);
        }
    };

    VStack previewStack(audioPreviewInner.position.x, audioPreviewInner.position.y + 38.f, audioPreviewInner.size.x, kUiSectionSpacingPx);
    previewBtn(previewStack.next(30.f), "UI CLICK", sf::Color(45, 90, 130), sfxClick.get(), 36.f, true);
    previewBtn(previewStack.next(30.f), "CONFIRM", sf::Color(45, 110, 85), sfxConfirm.get(), 44.f, true);
    previewBtn(previewStack.next(30.f), "CHAT OPEN", sf::Color(50, 95, 125), sfxChatOpen.get(), 36.f, true);
    previewBtn(previewStack.next(30.f), "VOTE TICK", sf::Color(85, 80, 125), sfxVoteTick.get(), 30.f, false);
    previewBtn(previewStack.next(30.f), "KILL STINGER", sf::Color(120, 55, 55), sfxDeath.get(), 62.f, false);
    previewBtn(previewStack.next(30.f), "PHASE TRANSITION", sf::Color(90, 90, 130), sfxPhase.get(), 40.f, false);

    sf::FloatRect ambMenuBtn({audioPreviewInner.position.x, audioPreviewPanel.position.y + audioPreviewPanel.size.y - 74.f},
                             {(audioPreviewInner.size.x - 6.f) * 0.5f, 30.f});
    sf::FloatRect ambGameBtn({ambMenuBtn.position.x + ambMenuBtn.size.x + 6.f, ambMenuBtn.position.y},
                             {ambMenuBtn.size.x, 30.f});
    drawButton(ambMenuBtn, "MENU AMB", sf::Color(54, 94, 120));
    drawButton(ambGameBtn, "GAME AMB", sf::Color(94, 70, 120));

    float now = uiClock.getElapsedTime().asSeconds();
    if (btnClicked(ambMenuBtn)) {
        previewMenuAmbUntil = now + 2.5f;
        previewGameAmbUntil = now;
    }
    if (btnClicked(ambGameBtn)) {
        previewGameAmbUntil = now + 2.5f;
        previewMenuAmbUntil = now;
    }
    if (audioEnabled && ambientMenuSound && ambientGameSound) {
        float base = (masterVolume / 100.f) * (ambientVolume / 100.f);
        if (now < previewMenuAmbUntil) {
            ambientMenuSound->setVolume(std::clamp(34.f * base, 0.f, 100.f));
            ambientGameSound->setVolume(std::clamp(6.f * base, 0.f, 100.f));
        } else if (now < previewGameAmbUntil) {
            ambientMenuSound->setVolume(std::clamp(3.f * base, 0.f, 100.f));
            ambientGameSound->setVolume(std::clamp(42.f * base, 0.f, 100.f));
        }
    }

    sf::FloatRect resetBtn({panelRect.position.x + 260.f, panelRect.position.y + panelRect.size.y - 64.f}, {220.f, 40.f});
    drawButton(resetBtn, "RESET DEFAULTS", sf::Color(80, 60, 40));
    if (btnClicked(resetBtn)) {
        nightDurationSec = 30.f;
        discussionDurationSec = 60.f;
        aiAccusationBias = 0;
        aiChatMinDelaySec = 3.f;
        aiChatMaxDelaySec = 8.f;
        masterVolume = 100.f;
        uiVolume = 100.f;
        sfxVolume = 100.f;
        ambientVolume = 100.f;
        audioMuted = false;
        masterVolumeBeforeMute = 100.f;
        settingsChanged = true;
    }

    if (masterVolume > 0.f && audioMuted) audioMuted = false;
    if (masterVolume <= 0.f && !audioMuted) audioMuted = true;

    // Back button
    sf::FloatRect backBtn({panelRect.position.x + panelRect.size.x - 250.f, panelRect.position.y + panelRect.size.y - 64.f}, {220.f, 40.f});
    drawButton(backBtn, "BACK", sf::Color(60, 60, 90));

    if (btnClicked(backBtn)) {
        if (settingsChanged) saveStats();
        transitionTo(Phase::MAIN_MENU);
    }

    if (settingsChanged) {
        saveStats();
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderLobby
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderLobby()
{
    drawBackdrop(sf::Color(10, 13, 18), sf::Color(50, 120, 160, 34));

    sf::FloatRect outerPanel = uiRect(0.05f, 0.094f, 0.90f, 0.814f);
    sf::FloatRect leftPanel = uiRect(0.064f, 0.156f, 0.381f, 0.708f);
    sf::FloatRect rightPanel = uiRect(0.458f, 0.156f, 0.477f, 0.708f);

    drawPanel(outerPanel, sf::Color(14, 18, 26, 230), sf::Color(80, 120, 170, 120));
    drawPanel(leftPanel, sf::Color(18, 22, 34, 240), sf::Color(120, 80, 80, 110));
    drawPanel(rightPanel, sf::Color(18, 22, 32, 240), sf::Color(80, 120, 160, 110));

    drawSectionHeader("LOBBY", "Configure the match before starting", uiY(0.117f), sf::Color(220, 90, 90));

    if (!fontLoaded) return;

    const float lineX = leftPanel.position.x + leftPanel.size.x * 0.08f;
    const float boxH = uiY(0.056f);
    VStack leftStack(lineX, leftPanel.position.y + leftPanel.size.y * 0.095f, leftPanel.size.x * 0.84f, kUiSectionSpacingPx);

    // â”€â”€ Name input â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        sf::FloatRect nameSlot = leftStack.next(uiY(0.042f) + boxH);
        const float y = nameSlot.position.y;
        sf::Text lbl(font, "Your Name:", uiFont(18));
        lbl.setFillColor(sf::Color(180,180,210));
        lbl.setPosition({lineX, y});
        window.draw(lbl);

        sf::FloatRect field({lineX, y + uiY(0.042f)}, {leftPanel.size.x * 0.57f, boxH});
        sf::RectangleShape box({field.size.x, field.size.y});
        box.setPosition({field.position.x, field.position.y});
        box.setFillColor(sf::Color(30,30,50));
        box.setOutlineColor(nameFocused ? sf::Color(100,160,255) : sf::Color(80,80,120));
        box.setOutlineThickness(2.f);
        window.draw(box);

        if (btnClicked(field)) nameFocused = true;

        std::string display = humanName + (nameFocused ? "|" : "");
        sf::Text txt(font, display.empty() ? " " : display, uiFont(18));
        txt.setFillColor(sf::Color::White);
        txt.setPosition({field.position.x + 6.f, field.position.y + 6.f});
        window.draw(txt);
    }

    // â”€â”€ Player count â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        sf::FloatRect countSlot = leftStack.next(uiY(0.044f) + boxH);
        const float y = countSlot.position.y;
        sf::Text lbl(font, "Players:", uiFont(18));
        lbl.setFillColor(sf::Color(180,180,210));
        lbl.setPosition({lineX, y});
        window.draw(lbl);

        const float buttonSize = uiY(0.05f);
        sf::FloatRect btnMinus({lineX, y + uiY(0.044f)}, {buttonSize, buttonSize});
        sf::FloatRect btnPlus ({lineX + buttonSize + uiX(0.04f), y + uiY(0.044f)}, {buttonSize, buttonSize});

        drawButton(btnMinus, "-", sf::Color(60,60,90));
        drawButton(btnPlus,  "+", sf::Color(60,60,90));

        sf::Text cnt(font, std::to_string(lobbyCount), uiFont(24));
        cnt.setFillColor(sf::Color::White);
        auto b = cnt.getLocalBounds();
        float countCenterX = lineX + buttonSize + uiX(0.02f);
        cnt.setPosition({countCenterX - b.size.x/2.f - b.position.x, y + uiY(0.046f)});
        snapTextToPixel(cnt);
        window.draw(cnt);

        if (btnClicked(btnMinus) && lobbyCount > 5)  { lobbyCount--; }
        if (btnClicked(btnPlus)  && lobbyCount < 10) { lobbyCount++; }
    }

    // â”€â”€ Role checkboxes â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    auto drawCheckbox = [&](float x, float y, float maxRight, bool& val, const std::string& label) -> float {
        const float boxSize = uiY(0.03f);
        const float labelSpacing = 12.f;
        x = std::round(x);
        y = std::round(y);
        sf::FloatRect box({x, y}, {boxSize, boxSize});
        sf::RectangleShape cb({boxSize, boxSize});
        cb.setPosition({x, y});
        cb.setFillColor(val ? sf::Color(60,180,60) : sf::Color(40,40,60));
        cb.setOutlineColor(sf::Color(100,100,140));
        cb.setOutlineThickness(1.f);
        window.draw(cb);
        if (val) {
            sf::Text tick(font, "x", uiFont(14));
            tick.setFillColor(sf::Color::White);
            tick.setPosition({std::round(x + boxSize * 0.2f), std::round(y + boxSize * 0.06f)});
            window.draw(tick);
        }
        sf::Text lbl(font, label, uiFont(16));
        lbl.setFillColor(sf::Color(200,200,220));
        const float labelX = x + boxSize + labelSpacing;
        fitTextToWidth(lbl, std::max(0.f, maxRight - labelX), 11);
        const auto lb = lbl.getLocalBounds();
        const float labelY = y + (boxSize - lb.size.y) * 0.5f - lb.position.y;
        lbl.setPosition({std::round(labelX), std::round(labelY)});
        window.draw(lbl);
        // Extend click area to include label
        const float clickWidth = std::max(boxSize, std::max(0.f, maxRight - x));
        sf::FloatRect clickArea({x, y}, {clickWidth, std::max(boxSize, lbl.getGlobalBounds().size.y + 6.f)});
        if (btnClicked(clickArea)) val = !val;

        return std::max(boxSize, lbl.getGlobalBounds().size.y + 8.f);
    };

    float leftCheckY = leftStack.currentY();
    float rightCheckY = leftStack.currentY();
    const float checkSpacing = kUiSectionSpacingPx;
    float leftCheckX = lineX;
    float rightCheckX = leftPanel.position.x + leftPanel.size.x * 0.55f;
    const float columnGap = 12.f;
    const float leftColumnRight = rightCheckX - columnGap;
    const float rightColumnRight = leftPanel.position.x + leftPanel.size.x - kUiContainerPaddingPx;

    leftCheckY += drawCheckbox(leftCheckX, leftCheckY, leftColumnRight, lobbyDet, "Include Detective") + checkSpacing;
    leftCheckY += drawCheckbox(leftCheckX, leftCheckY, leftColumnRight, lobbyDoc, "Include Doctor") + checkSpacing;
    leftCheckY += drawCheckbox(leftCheckX, leftCheckY, leftColumnRight, lobbyJoker, "Include Joker") + checkSpacing;

    rightCheckY += drawCheckbox(rightCheckX, rightCheckY, rightColumnRight, lobbyGodfather, "Include GodFather") + checkSpacing;
    rightCheckY += drawCheckbox(rightCheckX, rightCheckY, rightColumnRight, lobbySilencer, "Include Silencer") + checkSpacing;

    leftStack.setY(std::max(leftCheckY, rightCheckY) + kUiSectionSpacingPx);

    // â”€â”€ Role count preview â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        int n       = lobbyCount;
        int mafNum  = std::min(n / 3, 2); if (mafNum < 1) mafNum = 1;
        int special = mafNum + (lobbySilencer?1:0) + (lobbyDet?1:0)
                    + (lobbyDoc?1:0) + (lobbyJoker?1:0);
        int vils    = std::max(0, n - special);
        std::string preview = std::to_string(mafNum) + " Mafia";
        if (lobbyGodfather) preview += " (1 GodFather)";
        if (lobbySilencer)  preview += " Â· 1 Silencer";
        if (lobbyDet)   preview += " Â· 1 Detective";
        if (lobbyDoc)   preview += " Â· 1 Doctor";
        if (lobbyJoker) preview += " Â· 1 Joker";
        preview += " Â· " + std::to_string(vils) + " Villager" + (vils!=1?"s":"");

        sf::Text pv(font, wrapTextToWidth(font, uiFont(14), preview, leftPanel.size.x * 0.84f), uiFont(14));
        pv.setFillColor(sf::Color(160,160,200));
        pv.setPosition({lineX, leftStack.currentY()});
        window.draw(pv);

        leftStack.setY(pv.getGlobalBounds().position.y + pv.getGlobalBounds().size.y + kUiSectionSpacingPx);
    }

    // â”€â”€ Stats â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (stats.gamesPlayed > 0) {
        int winRate = stats.gamesPlayed > 0 ? (stats.wins * 100 / stats.gamesPlayed) : 0;
        std::string statStr = "Games: " + std::to_string(stats.gamesPlayed)
                            + "  |  Wins: " + std::to_string(stats.wins)
                            + "  |  Win rate: " + std::to_string(winRate) + "%";
        sf::Text st(font, wrapTextToWidth(font, uiFont(13), statStr, leftPanel.size.x * 0.84f), uiFont(13));
        st.setFillColor(sf::Color(100,200,100));
        st.setPosition({lineX, leftStack.currentY()});
        window.draw(st);

        leftStack.setY(st.getGlobalBounds().position.y + st.getGlobalBounds().size.y + kUiSectionSpacingPx);
    } else {
        leftStack.setY(leftStack.currentY() + kUiSectionSpacingPx);
    }

    // â”€â”€ START button â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        auto roleToIndex = [](Player::Role r) {
            return static_cast<int>(r);
        };

        auto buildPreviewRoles = [&]() {
            std::vector<Player::Role> roles;
            int n = lobbyCount;
            int mafiaNum = std::min(n / 3, 2);
            if (mafiaNum < 1) mafiaNum = 1;

            if (lobbyGodfather && mafiaNum >= 1) {
                roles.push_back(Player::Role::GODFATHER);
                for (int i = 1; i < mafiaNum; ++i) roles.push_back(Player::Role::MAFIA);
            } else {
                for (int i = 0; i < mafiaNum; ++i) roles.push_back(Player::Role::MAFIA);
            }
            if (lobbySilencer) roles.push_back(Player::Role::SILENCER);
            if (lobbyDet)      roles.push_back(Player::Role::DETECTIVE);
            if (lobbyDoc)      roles.push_back(Player::Role::DOCTOR);
            if (lobbyJoker)    roles.push_back(Player::Role::JOKER);
            while ((int)roles.size() < n) roles.push_back(Player::Role::VILLAGER);
            while ((int)roles.size() > n) roles.pop_back();
            return roles;
        };

        const std::vector<Player::Role> previewRoles = buildPreviewRoles();
        int roleCounts[8] = {};
        for (Player::Role r : previewRoles) {
            roleCounts[roleToIndex(r)]++;
        }

        const int mafiaAligned = roleCounts[roleToIndex(Player::Role::MAFIA)] +
                                 roleCounts[roleToIndex(Player::Role::GODFATHER)] +
                                 roleCounts[roleToIndex(Player::Role::SILENCER)];
        const int specialCount = roleCounts[roleToIndex(Player::Role::DETECTIVE)] +
                                 roleCounts[roleToIndex(Player::Role::DOCTOR)] +
                                 roleCounts[roleToIndex(Player::Role::JOKER)];
        const int townCount = roleCounts[roleToIndex(Player::Role::VILLAGER)];

        const float panelInset = 18.f;
        const float cardGap = 10.f;
        const float cardW = rightPanel.size.x - panelInset * 2.f;
        const float cardsTop = rightPanel.position.y + panelInset;
        const float cardsBottom = rightPanel.position.y + rightPanel.size.y - panelInset;

        const float summaryCardH = 70.f;
        const int previewRows = std::max(1, (lobbyCount + 1) / 2);
        const float rosterChipGapY = 8.f;
        const float rosterChipH = 20.f;
        const float rosterHeaderH = 44.f;
        const float rosterCardH = std::max(164.f, rosterHeaderH + previewRows * rosterChipH + (previewRows - 1) * rosterChipGapY + kUiContainerPaddingPx + 2.f);

        sf::FloatRect summaryCard(sf::Vector2f(rightPanel.position.x + panelInset, cardsTop),
                      sf::Vector2f(cardW, summaryCardH));
        sf::FloatRect rosterCard(sf::Vector2f(rightPanel.position.x + panelInset, summaryCard.position.y + summaryCard.size.y + cardGap),
                     sf::Vector2f(cardW, rosterCardH));
        const float mixY = rosterCard.position.y + rosterCard.size.y + cardGap;
        const float mixH = std::max(210.f, cardsBottom - mixY);
        sf::FloatRect mixCard(sf::Vector2f(rightPanel.position.x + panelInset, mixY),
                      sf::Vector2f(cardW, mixH));

        drawPanel(summaryCard, sf::Color(22, 27, 40, 230), sf::Color(95, 140, 190, 120));
        drawPanel(rosterCard, sf::Color(22, 27, 40, 230), sf::Color(95, 140, 190, 120));
        drawPanel(mixCard,    sf::Color(22, 27, 40, 230), sf::Color(95, 140, 190, 120));

        const sf::FloatRect summaryInner = insetRect(summaryCard);
        const sf::FloatRect rosterInner = insetRect(rosterCard);
        const sf::FloatRect mixInner = insetRect(mixCard);

        sf::Text summaryHdr(font, "MATCH SNAPSHOT", uiFont(17));
        summaryHdr.setFillColor(sf::Color(235, 205, 145));
        summaryHdr.setPosition({summaryInner.position.x, summaryInner.position.y});
        window.draw(summaryHdr);

        auto drawChip = [&](float x, float y, float w, const std::string& label, const std::string& value, sf::Color accent) {
            sf::RectangleShape chip({w, 24.f});
            chip.setPosition({x, y});
            chip.setFillColor(sf::Color(28, 33, 48, 180));
            chip.setOutlineColor(withAlpha(accent, 115));
            chip.setOutlineThickness(1.f);
            window.draw(chip);

            sf::Text valueText(font, value, uiFont(13));
            valueText.setFillColor(accent);
            fitTextToWidth(valueText, w * 0.45f, 10);
            valueText.setPosition({x + 10.f, y + 3.f});
            window.draw(valueText);

            sf::Text labelText(font, label, uiFont(12));
            labelText.setFillColor(sf::Color(190, 200, 215));
            fitTextToWidth(labelText, w * 0.45f, 10);
            auto lb = labelText.getLocalBounds();
            labelText.setPosition({x + w - 10.f - lb.size.x - lb.position.x, y + 4.f});
            window.draw(labelText);
        };

        const float chipY = summaryInner.position.y + 22.f;
        const float summaryGap = 6.f;
        const float summaryChipW = (summaryInner.size.x - summaryGap * 3.f) / 4.f;
        drawChip(summaryInner.position.x + (summaryChipW + summaryGap) * 0.f, chipY, summaryChipW, "SEATS", std::to_string(lobbyCount), sf::Color(120, 180, 255));
        drawChip(summaryInner.position.x + (summaryChipW + summaryGap) * 1.f, chipY, summaryChipW, "MAFIA", std::to_string(mafiaAligned), sf::Color(220, 90, 90));
        drawChip(summaryInner.position.x + (summaryChipW + summaryGap) * 2.f, chipY, summaryChipW, "SPECIAL", std::to_string(specialCount), sf::Color(180, 130, 255));
        drawChip(summaryInner.position.x + (summaryChipW + summaryGap) * 3.f, chipY, summaryChipW, "TOWN", std::to_string(townCount), sf::Color(110, 200, 130));

        sf::Text rosterHdr(font, "PLAYER PREVIEW", uiFont(18));
        rosterHdr.setFillColor(sf::Color(235, 205, 145));
        rosterHdr.setPosition({rosterInner.position.x, rosterInner.position.y});
        window.draw(rosterHdr);

        sf::Text rosterSub(font, "Human seat first, AI seats follow", uiFont(12));
        rosterSub.setFillColor(sf::Color(180, 190, 210));
        applyWrappedText(rosterSub, font, uiFont(12), "Human seat first, AI seats follow", rosterInner.size.x);
        rosterSub.setPosition({rosterInner.position.x, rosterInner.position.y + 22.f});
        window.draw(rosterSub);

        const float chipTop = rosterInner.position.y + 44.f;
        const float chipGapX = 10.f;
        const float chipGapY = 8.f;
        const int chipCols = 2;
        const float chipW = (rosterInner.size.x - chipGapX) / 2.f;
        const float chipH = 20.f;
        for (int i = 0; i < lobbyCount; ++i) {
            const int chipCol = i % chipCols;
            const int chipRow = i / chipCols;
            float chipX = rosterInner.position.x + chipCol * (chipW + chipGapX);
            float chipY = chipTop + chipRow * (chipH + chipGapY);

            sf::RectangleShape chip({chipW, chipH});
            chip.setPosition({chipX, chipY});
            chip.setFillColor(i == 0 ? sf::Color(34, 46, 70, 220) : sf::Color(28, 33, 48, 190));
            chip.setOutlineColor(i == 0 ? sf::Color(120, 180, 255, 140) : sf::Color(70, 90, 120, 60));
            chip.setOutlineThickness(1.f);
            window.draw(chip);

            sf::CircleShape dot(3.5f);
            dot.setPosition({chipX + 7.f, chipY + 7.f});
            dot.setFillColor(i == 0 ? sf::Color(120, 180, 255) : sf::Color(160, 160, 180));
            window.draw(dot);

            std::string playerName = (i == 0) ? (humanName.empty() ? "You" : humanName)
                                              : kAiNames[i - 1];
            sf::Text nameText(font, playerName, uiFont(12));
            nameText.setFillColor(sf::Color::White);
            fitTextToWidth(nameText, chipW - 76.f, 10);
            nameText.setPosition({chipX + 18.f, chipY + 1.f});
            window.draw(nameText);

            sf::Text tag(font, i == 0 ? "HUMAN" : "AI", uiFont(10));
            tag.setFillColor(i == 0 ? sf::Color(120, 180, 255) : sf::Color(170, 170, 185));
            fitTextToWidth(tag, 36.f, 9);
            auto tb = tag.getLocalBounds();
            tag.setPosition({chipX + chipW - 10.f - tb.size.x - tb.position.x, chipY + 2.f});
            window.draw(tag);
        }

        sf::Text mixHdr(font, "ROLE DISTRIBUTION", uiFont(18));
        mixHdr.setFillColor(sf::Color(235, 205, 145));
        mixHdr.setPosition({mixInner.position.x, mixInner.position.y});
        window.draw(mixHdr);

        sf::Text mixSub(font, "What the match can contain with the current toggles", uiFont(12));
        mixSub.setFillColor(sf::Color(180, 190, 210));
        applyWrappedText(mixSub, font, uiFont(12), "What the match can contain with the current toggles", mixInner.size.x);
        mixSub.setPosition({mixInner.position.x, mixInner.position.y + 22.f});
        window.draw(mixSub);

        struct MixRow { const char* label; Player::Role role; sf::Color color; };
        const MixRow mixRows[] = {
            {"Mafia",      Player::Role::MAFIA,      sf::Color(220, 70, 70)},
            {"GodFather",  Player::Role::GODFATHER,  sf::Color(160, 70, 120)},
            {"Silencer",   Player::Role::SILENCER,   sf::Color(170, 120, 255)},
            {"Doctor",     Player::Role::DOCTOR,     sf::Color(90, 190, 100)},
            {"Detective",  Player::Role::DETECTIVE,  sf::Color(90, 150, 240)},
            {"Joker",      Player::Role::JOKER,      sf::Color(240, 180, 70)},
            {"Villager",   Player::Role::VILLAGER,   sf::Color(170, 175, 190)}
        };

        const float barX = mixInner.position.x;
        const float barW = mixInner.size.x;
        const float countX = mixInner.position.x + mixInner.size.x - 2.f;
        float barY = mixInner.position.y + 44.f;
        for (const auto& row : mixRows) {
            int count = roleCounts[roleToIndex(row.role)];
            float ratio = lobbyCount > 0 ? static_cast<float>(count) / static_cast<float>(lobbyCount) : 0.f;

            sf::Text label(font, row.label, uiFont(13));
            label.setFillColor(sf::Color(220, 225, 235));
            fitTextToWidth(label, 94.f, 10);
            label.setPosition({barX, barY - 6.f});
            snapTextToPixel(label);
            window.draw(label);

            sf::RectangleShape track(sf::Vector2f(barW - 86.f, 8.f));
            track.setPosition({barX + 104.f, barY + 3.f});
            track.setFillColor(sf::Color(35, 40, 54));
            track.setOutlineColor(sf::Color(70, 80, 110));
            track.setOutlineThickness(1.f);
            window.draw(track);

            sf::RectangleShape fill(sf::Vector2f(std::max(0.f, (barW - 86.f) * ratio), 8.f));
            fill.setPosition({barX + 104.f, barY + 3.f});
            fill.setFillColor(row.color);
            window.draw(fill);

            sf::Text countText(font, std::to_string(count), uiFont(13));
            countText.setFillColor(row.color);
            auto cb = countText.getLocalBounds();
            countText.setPosition({countX - cb.size.x - cb.position.x, barY - 6.f});
            snapTextToPixel(countText);
            window.draw(countText);

            barY += 22.f;
        }

        sf::Text footerText(font, "Preview updates as you change the lobby toggles", uiFont(12));
        footerText.setFillColor(sf::Color(190, 205, 225));
        applyWrappedText(footerText, font, uiFont(12), "Preview updates as you change the lobby toggles", mixInner.size.x);
        auto fBounds = footerText.getLocalBounds();
        footerText.setPosition({mixInner.position.x,
                    mixCard.position.y + mixCard.size.y - kUiContainerPaddingPx - fBounds.size.y - fBounds.position.y});
        window.draw(footerText);

        bool canStart = !humanName.empty();
        sf::Color btnCol = canStart ? sf::Color(150,20,20) : sf::Color(60,60,60);
        const float startBtnH = uiY(0.069f);
        const float startY = std::min(leftStack.currentY(), leftPanel.position.y + leftPanel.size.y - startBtnH - kUiContainerPaddingPx);
        sf::FloatRect startRect({lineX, startY}, {leftPanel.size.x * 0.57f, startBtnH});
        drawButton(startRect, "START GAME", btnCol,
                   canStart ? sf::Color::White : sf::Color(120,120,120));
        if (canStart && btnClicked(startRect)) {
            assignRoles();
            transitionTo(Phase::ROLE_REVEAL);
        }
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderRoleReveal
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderRoleReveal()
{
    float t = roleRevealClock.getElapsedTime().asSeconds();
    window.clear(sf::Color(10,10,15));

    if (!fontLoaded) {
        return;
    }

    sf::RectangleShape focusDim({1280.f, 720.f});
    focusDim.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(focusDim);

    auto easeOutCubic = [](float v) {
        v = std::clamp(v, 0.f, 1.f);
        float inv = 1.f - v;
        return 1.f - inv * inv * inv;
    };

    float introT = easeOutCubic(t / 0.75f);
    float contentT = easeOutCubic((t - 0.35f) / 1.1f);
    std::uint8_t introAlpha = static_cast<std::uint8_t>(255.f * introT);
    std::uint8_t contentAlpha = static_cast<std::uint8_t>(255.f * contentT);
    float pulse = 1.f + 0.015f * std::sin(t * 4.f);

    // "YOUR ROLE IS..." fades in first to set up the reveal.
    {
        sf::Text intro(font, "YOUR ROLE IS...", 32);
        intro.setFillColor(sf::Color(245, 240, 230, introAlpha));
        auto b = intro.getLocalBounds();
        intro.setPosition({std::round(640.f - b.size.x/2.f - b.position.x), std::round(126.f + (1.f - introT) * 10.f)});
        window.draw(intro);
    }

    Player& h    = players[humanIdx];
    Player::Role r = h.getRole();

    const sf::Vector2f cardSize(400.f, 260.f);
    const sf::Vector2f cardCenter(640.f, cardY + cardSize.y * 0.5f);
    const float cardScale = (1.06f - 0.06f * contentT) * pulse;
    const sf::Vector2f scaledCardSize(cardSize.x * cardScale, cardSize.y * cardScale);
    const sf::Vector2f cardTopLeft(cardCenter.x - scaledCardSize.x * 0.5f,
                                   cardCenter.y - scaledCardSize.y * 0.5f);

    // Soft glow bloom behind the card.
    sf::CircleShape glow1(210.f * (0.95f + 0.05f * contentT));
    glow1.setOrigin({glow1.getRadius(), glow1.getRadius()});
    glow1.setPosition({cardCenter.x, cardCenter.y - 12.f});
    glow1.setFillColor(withAlpha(roleCardColor(r), static_cast<std::uint8_t>(28 + 38 * contentT)));
    window.draw(glow1);

    sf::CircleShape glow2(135.f * (0.92f + 0.08f * contentT));
    glow2.setOrigin({glow2.getRadius(), glow2.getRadius()});
    glow2.setPosition({cardCenter.x, cardCenter.y});
    glow2.setFillColor(withAlpha(sf::Color::White, static_cast<std::uint8_t>(12 + 18 * contentT)));
    window.draw(glow2);

    // Slide-in card
    sf::RectangleShape card(scaledCardSize);
    card.setOrigin({scaledCardSize.x * 0.5f, scaledCardSize.y * 0.5f});
    card.setPosition({std::round(cardCenter.x), std::round(cardCenter.y)});
    card.setFillColor(withAlpha(roleCardColor(r), static_cast<std::uint8_t>(230 * contentT + 20)));
    card.setOutlineColor(withAlpha(sf::Color(245, 230, 200), static_cast<std::uint8_t>(115 + 100 * contentT)));
    card.setOutlineThickness(2.f);
    window.draw(card);

    sf::Vector2f haloSize(scaledCardSize.x * 1.04f, scaledCardSize.y * 1.04f);
    sf::RectangleShape halo(haloSize);
    halo.setOrigin({haloSize.x * 0.5f, haloSize.y * 0.5f});
    halo.setPosition({std::round(cardCenter.x), std::round(cardCenter.y)});
    halo.setFillColor(sf::Color(0, 0, 0, 0));
    halo.setOutlineColor(withAlpha(roleCardColor(r), static_cast<std::uint8_t>(85 * contentT)));
    halo.setOutlineThickness(10.f);
    window.draw(halo);

    sf::Vector2f innerSize(scaledCardSize.x * 0.985f, scaledCardSize.y * 0.985f);
    sf::RectangleShape inner(innerSize);
    inner.setOrigin({innerSize.x * 0.5f, innerSize.y * 0.5f});
    inner.setPosition({std::round(cardCenter.x), std::round(cardCenter.y)});
    inner.setFillColor(sf::Color(255,255,255,0));
    inner.setOutlineColor(withAlpha(sf::Color(255,255,255), static_cast<std::uint8_t>(22 + 18 * contentT)));
    inner.setOutlineThickness(1.f);
    window.draw(inner);

    // Role name
    {
        sf::Text rn(font, h.getRoleName(), 48);
        rn.setFillColor(sf::Color(255, 250, 242, contentAlpha));
        auto b = rn.getLocalBounds();
        rn.setPosition({std::round(640.f - b.size.x/2.f - b.position.x), std::round(cardY + 18.f + (1.f - contentT) * 12.f)});
        window.draw(rn);
    }

    // Description
    {
        std::string wrappedDesc = wrapTextToWidth(font, 18, roleDesc(r), 330.f);
        sf::Text descText(font, wrappedDesc, 18);
        descText.setFillColor(sf::Color(220, 220, 225, contentAlpha));
        auto db = descText.getLocalBounds();
        descText.setOrigin({db.position.x + db.size.x * 0.5f, db.position.y});
        descText.setPosition({640.f, std::round(cardY + 88.f + (1.f - contentT) * 8.f)});
        window.draw(descText);
    }

    // MAFIA: list partners
    if (r == Player::Role::MAFIA) {
        std::string partners;
        for (auto& p : players) {
            if (!p.getIsHuman() && (p.getRole() == Player::Role::MAFIA ||
                                     p.getRole() == Player::Role::GODFATHER ||
                                     p.getRole() == Player::Role::SILENCER)) {
                if (!partners.empty()) partners += ", ";
                partners += p.getName();
            }
        }
        if (!partners.empty()) {
            std::string wrappedPartners = wrapTextToWidth(font, 16, "Partners: " + partners, 330.f);
            sf::Text pt(font, wrappedPartners, 16);
            pt.setFillColor(sf::Color(255, 180, 80, contentAlpha));
            auto b = pt.getLocalBounds();
            pt.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y});
            pt.setPosition({640.f, std::round(cardY + 156.f + (1.f - contentT) * 6.f)});
            window.draw(pt);
        }
    }

    // Blinking "Click anywhere to continue"
    if (blinkState && contentT > 0.7f) {
        sf::Text blink(font, "Click anywhere to continue", 20);
        blink.setFillColor(sf::Color(190, 195, 205, static_cast<std::uint8_t>(180.f * contentT)));
        auto b = blink.getLocalBounds();
        blink.setPosition({std::round(640.f - b.size.x/2.f - b.position.x), 564.f});
        window.draw(blink);
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderNight
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderNight()
{
    window.clear(sf::Color(5, 8, 22));
    renderNightSky();

    float elapsed = nightClock.getElapsedTime().asSeconds();
    float progress = 1.f - elapsed / nightDurationSec;
    drawProgressBar(progress, 0.f);

    // Reserve top space for HUD + phase labels to avoid overlap with cards.
    const sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);
    const float yPhaseTitle = safe.position.y + 74.f;
    const float yInstruction = yPhaseTitle + 44.f;
    const float yCards = yInstruction + 54.f;

    drawTopHud("NIGHT", sf::Color(90, 120, 220));
    drawCentered("NIGHT PHASE", 34, sf::Color(80,100,200), yPhaseTitle);
    drawRoundBadge();

    if (!fontLoaded) return;

    Player& h  = players[humanIdx];
    Player::Role role = h.getRole();

    // â”€â”€ Detective popup overlay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (detectPopupOpen && detectRevealIdx >= 0) {
        // Dim background
        sf::RectangleShape dim({1280.f, 720.f});
        dim.setFillColor(sf::Color(0,0,0,160));
        window.draw(dim);

        sf::RectangleShape popup({500.f, 200.f});
        popup.setPosition({390.f, 260.f});
        popup.setFillColor(sf::Color(30,30,60));
        popup.setOutlineColor(sf::Color(120,120,200));
        popup.setOutlineThickness(2.f);
        window.draw(popup);

        Player& target = players[detectRevealIdx];
        drawCentered(target.getName() + " is a " + target.getRoleName() + "!",
                     26, sf::Color(255,230,100), 285.f);

        sf::FloatRect gotIt({540.f, 390.f}, {200.f, 44.f});
        drawButton(gotIt, "Got it", sf::Color(40,80,40));
        if (btnClicked(gotIt)) {
            detectPopupOpen   = false;
            nightHumanTarget  = detectRevealIdx;
            // GodFather appears innocent
            Player::Role revealed = target.getRole();
            if (revealed == Player::Role::GODFATHER)
                revealed = Player::Role::VILLAGER;
            clues.push_back({ target.getName(), revealed });
            nightConfirmed    = true;
            resolveNight();
        }
        return;
    }

    // â”€â”€ Mafia UI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (role == Player::Role::MAFIA || role == Player::Role::GODFATHER) {
        drawCentered("Choose your target to KILL:", 22, sf::Color(220,80,80), yInstruction);

        float gx = 80.f, gy = yCards;
        int col = 0;
        for (int i = 0; i < (int)players.size(); ++i) {
            if (!players[i].getIsAlive()) continue;
            if (i == humanIdx) continue;
            if (isMafiaAlignedRole(players[i].getRole())) continue;

            sf::FloatRect cardR({gx, gy}, {200.f, 90.f});
            bool hov = btnHovered(cardR);
            bool sel = (nightHumanTarget == i);
            drawPlayerCard(i, gx, gy, sel, hov);
            if (btnClicked(cardR) && !nightConfirmed) nightHumanTarget = i;

            gx += 220.f; ++col;
            if (col == 4) { col = 0; gx = 80.f; gy += 110.f; }
        }

        if (nightHumanTarget >= 0 && !nightConfirmed) {
            std::string lbl = "KILL " + players[nightHumanTarget].getName() + "?";
            sf::FloatRect conf({490.f, 600.f}, {300.f, 50.f});
            drawButton(conf, lbl, sf::Color(140,20,20));
            if (btnClicked(conf)) { nightConfirmed = true; resolveNight(); }
        }
    }

    // â”€â”€ Silencer UI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    else if (role == Player::Role::SILENCER) {
        drawCentered("Choose who to SILENCE tonight:", 22, sf::Color(180,120,255), yInstruction);

        float gx = 80.f, gy = yCards; int col = 0;
        for (int i = 0; i < (int)players.size(); ++i) {
            if (!players[i].getIsAlive()) continue;
            if (i == humanIdx) continue;
            if (isMafiaAlignedRole(players[i].getRole())) continue;

            sf::FloatRect cardR({gx, gy}, {200.f, 90.f});
            bool hov = btnHovered(cardR);
            bool sel = (silencerTarget == i);
            drawPlayerCard(i, gx, gy, false, hov);
            if (sel) {
                sf::RectangleShape glow({200.f, 90.f});
                glow.setPosition({gx, gy});
                glow.setFillColor(sf::Color(0,0,0,0));
                glow.setOutlineColor(sf::Color(200,100,255));
                glow.setOutlineThickness(3.f);
                window.draw(glow);
            }
            if (btnClicked(cardR) && !nightConfirmed) silencerTarget = i;

            gx += 220.f; ++col;
            if (col == 4) { col = 0; gx = 80.f; gy += 110.f; }
        }

        if (silencerTarget >= 0 && !nightConfirmed) {
            std::string lbl = "SILENCE " + players[silencerTarget].getName() + "?";
            sf::FloatRect conf({490.f, 600.f}, {300.f, 50.f});
            drawButton(conf, lbl, sf::Color(90,35,120));
            if (btnClicked(conf)) { nightConfirmed = true; resolveNight(); }
        }
    }

    // â”€â”€ Doctor UI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    else if (role == Player::Role::DOCTOR) {
        drawCentered("Choose who to PROTECT tonight:", 22, sf::Color(80,200,80), yInstruction);

        float gx = 80.f, gy = yCards; int col = 0;
        for (int i = 0; i < (int)players.size(); ++i) {
            if (!players[i].getIsAlive()) continue;
            sf::FloatRect cardR({gx, gy}, {200.f, 90.f});
            bool hov = btnHovered(cardR);
            bool sel = (nightHumanTarget == i);
            // green tint for selected
            drawPlayerCard(i, gx, gy, false, hov);
            if (sel) {
                sf::RectangleShape glow({200.f, 90.f});
                glow.setPosition({gx, gy});
                glow.setFillColor(sf::Color(0,0,0,0));
                glow.setOutlineColor(sf::Color(50,220,50));
                glow.setOutlineThickness(3.f);
                window.draw(glow);
            }
            if (btnClicked(cardR) && !nightConfirmed) nightHumanTarget = i;
            gx += 220.f; ++col;
            if (col == 4) { col = 0; gx = 80.f; gy += 110.f; }
        }

        if (nightHumanTarget >= 0 && !nightConfirmed) {
            std::string lbl = "PROTECT " + players[nightHumanTarget].getName() + "?";
            sf::FloatRect conf({490.f, 600.f}, {300.f, 50.f});
            drawButton(conf, lbl, sf::Color(20,100,20));
            if (btnClicked(conf)) { nightConfirmed = true; resolveNight(); }
        }
    }

    // â”€â”€ Detective UI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    else if (role == Player::Role::DETECTIVE) {
        drawCentered("Click a player to INVESTIGATE:", 22, sf::Color(80,120,220), yInstruction);

        float gx = 80.f, gy = yCards; int col = 0;
        for (int i = 0; i < (int)players.size(); ++i) {
            if (!players[i].getIsAlive()) continue;
            if (i == humanIdx) continue;
            sf::FloatRect cardR({gx, gy}, {200.f, 90.f});
            bool hov = btnHovered(cardR);
            drawPlayerCard(i, gx, gy, false, hov);
            if (btnClicked(cardR) && !nightConfirmed) {
                detectRevealIdx = i;
                detectPopupOpen = true;
            }
            gx += 220.f; ++col;
            if (col == 4) { col = 0; gx = 80.f; gy += 110.f; }
        }

        // Previously gathered clues
        if (!clues.empty()) {
            sf::Text cl(font, "Your clues:", 14);
            cl.setFillColor(sf::Color(150,150,200));
            cl.setPosition({900.f, yCards});
            window.draw(cl);
            float cy2 = yCards + 25.f;
            for (auto& [nm, rl] : clues) {
                Player tmp; tmp.setRole(rl);
                sf::Text c2(font, wrapTextToWidth(font, 13, nm + ": " + tmp.getRoleName(), 320.f), 13);
                c2.setFillColor(sf::Color(200,200,255));
                c2.setPosition({900.f, cy2});
                snapTextToPixel(c2);
                window.draw(c2);
                cy2 += c2.getGlobalBounds().size.y + 4.f;
            }
        }
    }

    // â”€â”€ Villager / Joker â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    else {
        float calmPulse = 1.f + 0.012f * std::sin(elapsed * 1.25f);
        float calmFade = 0.72f + 0.10f * (0.5f + 0.5f * std::sin(elapsed * 0.65f));

        const unsigned idleBaseSize = uiFont(26);
        const unsigned idleSize = std::max(10u, static_cast<unsigned>(std::round(idleBaseSize * calmPulse)));
        sf::Text idleText(font, "You have no night action.", idleSize);
        idleText.setFillColor(sf::Color(130, 138, 168, static_cast<std::uint8_t>(220.f * calmFade)));
        auto idleBounds = idleText.getLocalBounds();
        idleText.setPosition({640.f - idleBounds.size.x / 2.f - idleBounds.position.x,
                              yInstruction + 32.f});
        snapTextToPixel(idleText);
        window.draw(idleText);

        const float sleepPulse = 1.f + 0.008f * std::sin(elapsed * 1.05f);
        const unsigned sleepBaseSize = uiFont(20);
        const unsigned sleepSize = std::max(10u, static_cast<unsigned>(std::round(sleepBaseSize * sleepPulse)));
        sf::Text sleepText(font, "The town sleeps...", sleepSize);
        sleepText.setFillColor(sf::Color(92, 98, 132, static_cast<std::uint8_t>(200.f * calmFade)));
        auto sleepBounds = sleepText.getLocalBounds();
        sleepText.setPosition({640.f - sleepBounds.size.x / 2.f - sleepBounds.position.x,
                               yInstruction + 78.f + std::sin(elapsed * 0.9f) * 2.f});
        snapTextToPixel(sleepText);
        window.draw(sleepText);

        // Zzz animation
        float zt = std::fmod(zzzClock.getElapsedTime().asSeconds(), 2.7f);
        for (int i = 0; i < 3; ++i) {
            float phase2 = static_cast<float>(i) * 0.34f;
            float local = zt - phase2;
            if (local < 0.f || local > 1.35f) continue;

            float rise = local / 1.35f;
            float alpha = std::sin(std::clamp(rise, 0.f, 1.f) * 3.1415926f);
            float yOff = -rise * 26.f;
            float xOff = std::sin(elapsed * 0.75f + i) * 1.5f;

            sf::Text z(font, std::string(i + 1, 'Z'), static_cast<unsigned>(18 + i * 5));
            z.setFillColor(sf::Color(165, 165, 225, static_cast<std::uint8_t>(alpha * 190.f)));
            z.setPosition({612.f + i * 22.f + xOff, 330.f + yOff});
            window.draw(z);
        }

        sf::FloatRect skip({490.f, 600.f}, {300.f, 50.f});
        drawButton(skip, "Sleep (skip)", sf::Color(50,50,80));
        if (btnClicked(skip) && !nightConfirmed) {
            nightConfirmed = true;
            resolveNight();
        }
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderDay
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderDay()
{
    drawBackdrop(sf::Color(20, 16, 10), sf::Color(220, 160, 40, 30));
    drawPanel(sf::FloatRect({60.f, 74.f}, {1160.f, 590.f}), sf::Color(22, 20, 16, 235), sf::Color(200, 150, 60, 120));
    // UI Text overlay (moved to layer 4 below)

    float phaseTime = phaseAnimClock.getElapsedTime().asSeconds();

    // Big announcement
    sf::Color annCol = (dayMsg.find("NO ONE") != std::string::npos)
                     ? sf::Color(100,200,100) : sf::Color(255,140,0);
    float dayTextY = uiY(0.23f);
    {
        const float resultDelay = 0.35f;
        float revealT = std::clamp((phaseTime - resultDelay) / 0.9f, 0.f, 1.f);
        float revealEase = 1.f - std::pow(1.f - revealT, 3.f);
        float pulse = 1.f + 0.03f * std::sin(phaseTime * 3.2f) * revealEase;

        sf::Color accent = annCol;
        sf::FloatRect msgPanel({uiX(0.14f), uiY(0.19f)}, {uiX(0.72f), uiY(0.16f)});
        drawPanel(msgPanel,
                  sf::Color(28, 22, 16, static_cast<std::uint8_t>(120 + 90 * revealEase)),
                  withAlpha(accent, static_cast<std::uint8_t>(90 + 130 * revealEase)));

        sf::RectangleShape glow({msgPanel.size.x + 28.f, msgPanel.size.y + 20.f});
        glow.setPosition({msgPanel.position.x - 14.f, msgPanel.position.y - 10.f});
        glow.setFillColor(sf::Color(0, 0, 0, 0));
        glow.setOutlineColor(withAlpha(accent, static_cast<std::uint8_t>(40 + 70 * revealEase)));
        glow.setOutlineThickness(4.f);
        window.draw(glow);

        if (phaseTime < resultDelay) {
            sf::Text hold(font, "Dawn arrives...", uiFont(22));
            hold.setFillColor(sf::Color(180, 160, 120, 150));
            auto hb = hold.getLocalBounds();
            hold.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y});
            hold.setPosition({uiX(0.5f), msgPanel.position.y + 22.f});
            snapTextToPixel(hold);
            window.draw(hold);
        }

        const unsigned annBaseSize = uiFont(40);
        const unsigned annSize = std::max(10u, static_cast<unsigned>(std::round(annBaseSize * pulse)));
        sf::Text ann(font, wrapTextToWidth(font, annSize, dayMsg, uiX(0.68f)), annSize);
        ann.setFillColor(withAlpha(annCol, static_cast<std::uint8_t>(255 * revealEase)));
        auto ab = ann.getLocalBounds();
        ann.setOrigin({ab.position.x + ab.size.x * 0.5f, ab.position.y + ab.size.y * 0.5f});
        ann.setPosition({uiX(0.5f), msgPanel.position.y + msgPanel.size.y * 0.5f + 4.f});
        snapTextToPixel(ann);
        if (revealT > 0.f) window.draw(ann);

        dayTextY = msgPanel.position.y + msgPanel.size.y + uiY(0.014f);
    }

    // Show last will of killed player
    if (lastKilledIdx >= 0 && lastKilledIdx < (int)lastWills.size() &&
        !lastWills[lastKilledIdx].empty()) {
        sf::Text willHdr(font, "LAST WILL:", uiFont(16));
        willHdr.setFillColor(sf::Color(200, 200, 140));
        auto hb = willHdr.getLocalBounds();
        willHdr.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y});
        willHdr.setPosition({uiX(0.5f), dayTextY});
        snapTextToPixel(willHdr);
        window.draw(willHdr);
        dayTextY += willHdr.getGlobalBounds().size.y + uiY(0.01f);

        sf::Text willText(font, wrapTextToWidth(font, uiFont(15), "\"" + lastWills[lastKilledIdx] + "\"", uiX(0.78f)), uiFont(15));
        willText.setFillColor(sf::Color(180, 180, 160));
        auto wb = willText.getLocalBounds();
        willText.setOrigin({wb.position.x + wb.size.x * 0.5f, wb.position.y});
        willText.setPosition({uiX(0.5f), dayTextY});
        snapTextToPixel(willText);
        window.draw(willText);
        dayTextY += willText.getGlobalBounds().size.y + uiY(0.018f);
    }

    // Player grid (all players, dead greyed)
    float gx = 60.f, gy = std::max(uiY(0.30f), dayTextY); int col = 0;
    int cardSlot = 0;
    for (int i = 0; i < (int)players.size(); ++i) {
        (void)players[i].getIsAlive();
        float reveal = std::clamp((phaseTime - 0.06f * cardSlot) * 2.2f, 0.f, 1.f);
        float yAnim = (1.f - reveal) * 26.f;
        drawPlayerCard(i, gx, gy + yAnim, false, false);
        gx += 220.f; col++;
        cardSlot++;
        if (col == 5) { col = 0; gx = 60.f; gy += 110.f; }
    }

    // â”€â”€ Layer 4: UI Text Overlay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    drawTopHud("DAY", sf::Color(230, 170, 70));
    drawSectionHeader("DAY PHASE", "Review evidence, inspect roles, and prepare the vote", uiY(0.108f), sf::Color(230, 170, 60));
    drawRoundBadge();

    sf::FloatRect cont({490.f, 640.f}, {300.f, 50.f});
    drawButton(cont, "Continue to Discussion", sf::Color(40,60,100));
    if (btnClicked(cont)) transitionTo(Phase::DISCUSSION);
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderDiscussion
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderDiscussion()
{
    drawBackdrop(sf::Color(12,12,18), sf::Color(60,160,160,28));
    const sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);

    float phaseTime = phaseAnimClock.getElapsedTime().asSeconds();
    float panelReveal = std::clamp(phaseTime * 2.4f, 0.f, 1.f);
    float leftPanelOffset = (1.f - panelReveal) * -20.f;
    float rightPanelOffset = (1.f - panelReveal) * 20.f;

    float panelGap = 20.f;
    const float topContentY = safe.position.y + 146.f;
    float panelY = topContentY;
    float panelH = std::max(260.f, safe.size.y - (panelY - safe.position.y) - 18.f);
    float panelW = (safe.size.x - panelGap) * 0.5f;
    sf::FloatRect leftPanel({safe.position.x, panelY}, {panelW, panelH});
    sf::FloatRect rightPanel({safe.position.x + panelW + panelGap, panelY}, {panelW, panelH});
    leftPanel.position.x += leftPanelOffset;
    rightPanel.position.x += rightPanelOffset;

    drawPanel(leftPanel, sf::Color(16, 18, 26, 240), sf::Color(70, 140, 170, 120));
    drawPanel(rightPanel, sf::Color(16, 18, 26, 240), sf::Color(70, 140, 170, 120));

    float elapsed = discClock.getElapsedTime().asSeconds();
    int secsLeft = std::max(0, static_cast<int>(discussionDurationSec - elapsed));

    if (!fontLoaded) return;

    std::string activeSpeaker;
    {
        std::lock_guard<std::mutex> lock(chatMutex);
        if (!chat.empty()) activeSpeaker = chat.back().sender;
    }

    // Visual separator between players and chat columns.
    sf::RectangleShape divider({4.f, rightPanel.size.y - 24.f});
    divider.setPosition({leftPanel.position.x + leftPanel.size.x + panelGap * 0.5f - 2.f, rightPanel.position.y + kUiContainerPaddingPx});
    divider.setFillColor(withAlpha(sf::Color(90, 150, 180), static_cast<std::uint8_t>(90 + 35 * std::sin(uiClock.getElapsedTime().asSeconds() * 2.2f))));
    window.draw(divider);

    sf::Text playersHdr(font, "PLAYERS", uiFont(13));
    playersHdr.setFillColor(sf::Color(150, 185, 205));
    placeTextTopLeft(playersHdr, leftPanel.position.x + kUiContainerPaddingPx, leftPanel.position.y + kUiContainerPaddingPx);
    window.draw(playersHdr);

    sf::Text chatHdr(font, "TABLE CHAT", uiFont(13));
    chatHdr.setFillColor(sf::Color(155, 210, 215));
    placeTextTopLeft(chatHdr, rightPanel.position.x + kUiContainerPaddingPx, rightPanel.position.y + kUiContainerPaddingPx);
    window.draw(chatHdr);

    // â”€â”€ Left: alive player cards â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    float gx = leftPanel.position.x + uiX(0.002f);
    float gy = leftPanel.position.y + uiY(0.022f);
    int col = 0;
    int cardSlot = 0;
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive()) continue;
        float reveal = std::clamp((elapsed - 0.07f * cardSlot) * 3.0f, 0.f, 1.f);
        float xAnim = (1.f - reveal) * -uiX(0.019f);
        float cardX = gx + xAnim;
        drawPlayerCard(i, cardX, gy, false, false);

        bool isActiveSpeaker = !activeSpeaker.empty() && players[i].getName() == activeSpeaker;
        if (isActiveSpeaker) {
            float pulse = 0.5f + 0.5f * std::sin(uiClock.getElapsedTime().asSeconds() * 5.f);
            sf::RectangleShape glow({208.f, 98.f});
            glow.setPosition({cardX - 4.f, gy - 4.f});
            glow.setFillColor(sf::Color(0, 0, 0, 0));
            glow.setOutlineColor(sf::Color(120, 230, 230, static_cast<std::uint8_t>(120 + pulse * 100.f)));
            glow.setOutlineThickness(2.f);
            window.draw(glow);

            sf::Text speaking(font, "SPEAKING", 11);
            speaking.setFillColor(sf::Color(130, 230, 230));
            placeTextTopLeft(speaking, cardX + 120.f, gy + 6.f);
            window.draw(speaking);
        }

        gx += uiX(0.164f); col++;
        cardSlot++;
        if (col == 3) {
            col = 0;
            gx = leftPanel.position.x + uiX(0.002f);
            gy += uiY(0.146f);
        }
    }

    // â”€â”€ Right: chat panel â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    float cx = rightPanel.position.x + uiX(0.016f);
    float cy2 = rightPanel.position.y + uiY(0.022f);
    float cw = rightPanel.size.x - uiX(0.018f);
    float ch = rightPanel.size.y - uiY(0.095f);

    sf::RectangleShape chatBackdrop({cw + 10.f, ch + 12.f});
    chatBackdrop.setPosition({cx - 5.f, cy2 - 6.f});
    chatBackdrop.setFillColor(sf::Color(14, 20, 30, 120));
    chatBackdrop.setOutlineColor(sf::Color(70, 120, 145, 90));
    chatBackdrop.setOutlineThickness(1.f);
    window.draw(chatBackdrop);

    for (int s = 0; s < 5; ++s) {
        sf::RectangleShape stripe({cw - 10.f, 1.f});
        stripe.setPosition({cx + 5.f, cy2 + 16.f + s * ((ch - 56.f) / 5.f)});
        stripe.setFillColor(sf::Color(95, 120, 155, 16));
        window.draw(stripe);
    }

    sf::RectangleShape panel({cw, ch});
    panel.setPosition({cx, cy2});
    panel.setFillColor(sf::Color(18,18,28));
    panel.setOutlineColor(sf::Color(60,60,100));
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    // Messages (draw last lines that fit using bounds-based vertical layout)
    const float messageAreaTop = cy2 + 6.f;
    const float messageAreaBottom = cy2 + ch - 44.f;
    const float messageSpacing = 12.f;

    std::vector<ChatMsg> chatSnapshot;
    {
        std::lock_guard<std::mutex> lock(chatMutex);
        chatSnapshot = chat;
    }

    std::vector<float> rowHeights(chatSnapshot.size(), 0.f);
    float totalHeight = 0.f;
    for (int i = static_cast<int>(chatSnapshot.size()) - 1; i >= 0; --i) {
        sf::Text senderMeasure(font, chatSnapshot[i].sender + ": ", 13);
        float senderW = senderMeasure.getLocalBounds().size.x + senderMeasure.getLocalBounds().position.x + 2.f;
        float bodyMaxW = std::max(60.f, (cx + cw - 8.f) - (cx + 4.f + senderW));
        std::string wrappedBody = wrapTextToWidth(font, 13, chatSnapshot[i].text, bodyMaxW);
        sf::Text bodyMeasure(font, wrappedBody, 13);
        float rowH = std::max(senderMeasure.getGlobalBounds().size.y, bodyMeasure.getGlobalBounds().size.y) + messageSpacing;
        rowHeights[i] = rowH;
        if (totalHeight + rowH > (messageAreaBottom - messageAreaTop)) {
            break;
        }
        totalHeight += rowH;
    }

    float usedHeight = 0.f;
    int start = static_cast<int>(chatSnapshot.size());
    for (int i = static_cast<int>(chatSnapshot.size()) - 1; i >= 0; --i) {
        if (usedHeight + rowHeights[i] > totalHeight + 0.01f) {
            break;
        }
        usedHeight += rowHeights[i];
        start = i;
    }

    float ly = messageAreaBottom - totalHeight;
    for (int i = start; i < (int)chatSnapshot.size(); ++i) {
        auto& m = chatSnapshot[i];
        int visibleIdx = i - start;
        float reveal = std::clamp((elapsed * 1.35f) - visibleIdx * 0.09f, 0.f, 1.f);
        float yAnim = (1.f - reveal) * 8.f;
        bool isActiveLine = !activeSpeaker.empty() && m.sender == activeSpeaker;

        sf::RectangleShape bubble({cw - 12.f, std::max(18.f, rowHeights[i] - 3.f)});
        bubble.setPosition({cx + 3.f, ly - 1.f + yAnim});
        bubble.setFillColor(isActiveLine ? sf::Color(26, 44, 56, static_cast<std::uint8_t>(110 * reveal))
                                         : sf::Color(22, 26, 38, static_cast<std::uint8_t>(80 * reveal)));
        bubble.setOutlineColor(isActiveLine ? sf::Color(110, 200, 210, static_cast<std::uint8_t>(120 * reveal))
                                            : sf::Color(70, 90, 120, static_cast<std::uint8_t>(80 * reveal)));
        bubble.setOutlineThickness(1.f);
        window.draw(bubble);

        sf::Text sender(font, m.sender + ": ", 13);
        sender.setFillColor(isActiveLine ? sf::Color(170, 235, 235, static_cast<std::uint8_t>(255 * reveal))
                                         : withAlpha(m.color, static_cast<std::uint8_t>(255 * reveal)));
        placeTextTopLeft(sender, cx + 8.f, ly + yAnim);
        window.draw(sender);
        float textX = cx + 4.f + sender.getLocalBounds().size.x
                  + sender.getLocalBounds().position.x + 2.f;
        float bodyMaxW = std::max(60.f, (cx + cw - 8.f) - textX);
        sf::Text body(font, wrapTextToWidth(font, 13, m.text, bodyMaxW), 13);
        body.setFillColor(sf::Color(200,200,200, static_cast<std::uint8_t>(255 * reveal)));
        placeTextTopLeft(body, textX + 4.f, ly + yAnim);
        window.draw(body);
        float rowH = std::max(sender.getGlobalBounds().size.y, body.getGlobalBounds().size.y) + messageSpacing;
        ly += rowH;
    }

    // Input box
    sf::RectangleShape inputBox({cw - 8.f, 34.f});
    inputBox.setPosition({cx + 4.f, cy2 + ch - 40.f});
    inputBox.setFillColor(sf::Color(28,28,44));
    inputBox.setOutlineColor(sf::Color(80,80,130));
    inputBox.setOutlineThickness(1.f);
    window.draw(inputBox);

    if (players[humanIdx].isSilenced()) {
        sf::Text inp(font, "You have been SILENCED", 14);
        inp.setFillColor(sf::Color(200,80,255));
        placeTextTopLeft(inp, cx + 8.f, cy2 + ch - 36.f);
        window.draw(inp);
    } else {
        bool showCursor = ((int)(uiClock.getElapsedTime().asSeconds() * 2.f) % 2) == 0;
        sf::Text inp(font, chatInput + (showCursor ? "|" : " "), 14);
        inp.setFillColor(sf::Color::White);
        placeTextTopLeft(inp, cx + 8.f, cy2 + ch - 36.f);
        window.draw(inp);
    }

    // â”€â”€ Layer 4: UI Text Overlay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    drawProgressBar(1.f - elapsed / discussionDurationSec, 0.f);
    drawTopHud("DISCUSSION", sf::Color(70, 190, 190));
    drawSectionHeader("DISCUSSION", "Use chat, clues, and vote pressure to shape the table", uiY(0.108f), sf::Color(70, 190, 190));
    drawRoundBadge();
    drawCentered(std::to_string(secsLeft) + "s", uiFont(18), sf::Color(160,160,180), safe.position.y + 126.f);

    // Skip to vote button
    sf::FloatRect skip({cx, cy2 + ch + uiY(0.011f)}, {cw, uiY(0.061f)});
    drawButton(skip, "Skip to Vote", sf::Color(80,40,40));
    if (btnClicked(skip)) transitionTo(Phase::VOTING);
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderVoting
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderVoting()
{
    drawBackdrop(sf::Color(18,8,8), sf::Color(200, 60, 60, 28));
    const sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);
    float phaseTime = phaseAnimClock.getElapsedTime().asSeconds();
    float panelReveal = std::clamp(phaseTime * 2.8f, 0.f, 1.f);
    float panelYOffset = (1.f - panelReveal) * 18.f;
    const float topContentY = safe.position.y + 146.f;
    drawPanel(sf::FloatRect({safe.position.x, topContentY + panelYOffset}, {safe.size.x, std::max(250.f, safe.size.y - (topContentY - safe.position.y) - 18.f)}), sf::Color(18, 14, 20, 240), sf::Color(180, 70, 70, 120));
    // UI Text overlay (moved to layer 4 below)

    if (!fontLoaded) return;

    // Large vote cards (220x100)
    float gx = safe.position.x + 20.f, gy = topContentY + 20.f + panelYOffset; int col = 0;
    int cardSlot = 0;
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive()) continue;

        float reveal = std::clamp((phaseTime - 0.05f * cardSlot) * 2.8f, 0.f, 1.f);
        float yAnim = (1.f - reveal) * 24.f;

        sf::FloatRect cardR({gx, gy + yAnim}, {220.f, 100.f});
        bool hov = !humanVoted && btnHovered(cardR);
        bool sel = (humanVoteTarget == i);
        bool isElim = (showElim && i == eliminatedIdx);
        float pulse = 0.5f + 0.5f * std::sin(uiClock.getElapsedTime().asSeconds() * 5.f + i * 0.5f);

        // Shadow
        sf::RectangleShape shadow({226.f, 106.f});
        shadow.setPosition({gx + 3.f, gy + yAnim + 4.f});
        shadow.setFillColor(sf::Color(0, 0, 0, 95));
        window.draw(shadow);

        // Draw bigger card
        sf::RectangleShape card({220.f, 100.f});
        card.setPosition({gx, gy + yAnim});
        if (isElim) {
            card.setFillColor(sf::Color(
                static_cast<std::uint8_t>(120 + pulse * 20), 0,
                static_cast<std::uint8_t>(6 + pulse * 8)
            ));
            card.setOutlineColor(sf::Color(255, static_cast<std::uint8_t>(50 + pulse * 30), static_cast<std::uint8_t>(50 + pulse * 30)));
        } else if (sel) {
            card.setFillColor(sf::Color(
                static_cast<std::uint8_t>(82 + pulse * 26),
                static_cast<std::uint8_t>(26 + pulse * 10),
                static_cast<std::uint8_t>(30 + pulse * 10)
            ));
            card.setOutlineColor(sf::Color(235, static_cast<std::uint8_t>(90 + pulse * 25), static_cast<std::uint8_t>(90 + pulse * 25)));
        } else if (hov) {
            card.setFillColor(sf::Color(
                static_cast<std::uint8_t>(60 + pulse * 15),
                static_cast<std::uint8_t>(60 + pulse * 15),
                static_cast<std::uint8_t>(90 + pulse * 20)
            ));
            card.setOutlineColor(sf::Color(170, 170, static_cast<std::uint8_t>(200 + pulse * 20)));
        } else {
            card.setFillColor(sf::Color(35,35,55));
            card.setOutlineColor(sf::Color(80,80,120));
        }
        card.setOutlineThickness(2.f);
        window.draw(card);

        if (sel || isElim) {
            sf::RectangleShape glow({228.f, 108.f});
            glow.setPosition({gx - 4.f, gy + yAnim - 4.f});
            glow.setFillColor(sf::Color(0,0,0,0));
            glow.setOutlineThickness(3.f);
            glow.setOutlineColor(sf::Color(255, 110, 110, static_cast<std::uint8_t>(90 + pulse * 100)));
            window.draw(glow);

            if (isElim && showElim && !tallyAnim) {
                float doomPulse = 0.5f + 0.5f * std::sin(uiClock.getElapsedTime().asSeconds() * 7.f);
                sf::RectangleShape aura({246.f, 126.f});
                aura.setPosition({gx - 13.f, gy + yAnim - 13.f});
                aura.setFillColor(sf::Color(0,0,0,0));
                aura.setOutlineThickness(5.f);
                aura.setOutlineColor(sf::Color(255, 70, 70, static_cast<std::uint8_t>(110 + doomPulse * 110.f)));
                window.draw(aura);

                sf::Text doomed(font, "ELIMINATED", 12);
                doomed.setFillColor(sf::Color(255, 105, 105));
                auto db = doomed.getLocalBounds();
                placeTextTopLeft(doomed, gx + 110.f - db.size.x * 0.5f, gy + yAnim - 16.f);
                window.draw(doomed);
            }
        }

        // Avatar circle
        sf::CircleShape av(16.f);
        av.setFillColor(kColors[i % 8]);
        av.setPosition({gx + 8.f, gy + 8.f + yAnim});
        window.draw(av);

        // Name
        sf::Text nm(font, players[i].getName(), 16);
        nm.setFillColor(sf::Color::White);
        auto b = nm.getLocalBounds();
        placeTextTopLeft(nm, gx + 110.f - b.size.x * 0.5f, gy + 34.f + yAnim);
        window.draw(nm);

        // Tally
        int shown = (i < (int)tallyShown.size()) ? tallyShown[i] : 0;
        if (votesDone && shown > 0) {
            std::string vs = std::to_string(shown) + " vote" + (shown!=1?"s":"");
            sf::Text vt(font, vs, 13);
            vt.setFillColor(sf::Color(255,200,0));
            auto vb = vt.getLocalBounds();
            placeTextTopLeft(vt, gx + 110.f - vb.size.x * 0.5f, gy + 58.f + yAnim);
            window.draw(vt);
        }

        if (!humanVoted && btnClicked(cardR)) {
            humanVoteTarget = i;
            showConfirm     = true;
        }

        gx += 240.f; col++;
        cardSlot++;
        if (col == 4) { col = 0; gx = safe.position.x + 20.f; gy += 120.f; }
    }

    // â”€â”€ Vote reveal panel (right side) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (votesDone && fontLoaded) {
        float vy = topContentY + 8.f;
        float voteColX = safe.position.x + safe.size.x - 220.f;
        sf::Text hdr(font, "VOTES CAST", 18);
        hdr.setFillColor(sf::Color(255, 200, 100));
        placeTextTopLeft(hdr, voteColX, vy);
        window.draw(hdr);
        vy += 30.f;

        // Human vote
        if (humanVoteTarget >= 0) {
            sf::Text hv(font, players[humanIdx].getName() + " > "
                        + players[humanVoteTarget].getName(), 13);

            hv.setFillColor(sf::Color::White);
            placeTextTopLeft(hv, voteColX, vy);
            window.draw(hv);
            vy += 20.f;
        }

        // Revealed AI votes
        for (int i = 0; i <voteRevealIdx && i < (int)voteReveals.size(); ++i) {
            auto& vr = voteReveals[i];
            sf::Text vt(font, players[vr.voter].getName() + " > "
                        + players[vr.target].getName(), 13);
            vt.setFillColor(kColors[vr.voter % 10]);
            placeTextTopLeft(vt, voteColX, vy);
            window.draw(vt);
            vy += 20.f;
        }

        // Current reveal announcement
        if (revealingVotes && voteRevealIdx > 0 && voteRevealIdx <= (int)voteReveals.size()) {
            auto& last = voteReveals[voteRevealIdx - 1];
            std::string ann = players[last.voter].getName() + " votes for "
                            + players[last.target].getName() + "!";
            float revT = std::clamp(voteRevealClock.getElapsedTime().asSeconds() / 0.5f, 0.f, 1.f);
            float revScale = 1.03f - 0.03f * revT;
            std::uint8_t revAlpha = static_cast<std::uint8_t>(255.f * (1.f - 0.2f * revT));
            const unsigned revealBaseSize = uiFont(20);
            const unsigned revealSize = std::max(10u, static_cast<unsigned>(std::round(revealBaseSize * revScale)));
            sf::Text annText(font, wrapTextToWidth(font, revealSize, ann, uiX(0.8f)), revealSize);
            annText.setFillColor(sf::Color(255, 220, 80, revAlpha));
            auto ab = annText.getLocalBounds();
            annText.setOrigin({ab.position.x + ab.size.x * 0.5f, ab.position.y + ab.size.y * 0.5f});
            annText.setPosition({uiX(0.5f), uiY(0.915f)});
            snapTextToPixel(annText);
            window.draw(annText);
        } else if (revealingVotes && voteRevealIdx == 0) {
            drawCentered("Counting votes...", uiFont(20), sf::Color(180, 180, 100), uiY(0.915f));
        }
    }

    // â”€â”€ Layer 4: UI Text Overlay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    drawTopHud("VOTING", sf::Color(220, 80, 80));
    drawSectionHeader("VOTING", "Watch the reveal and read the room", uiY(0.108f), sf::Color(220, 80, 80));
    drawRoundBadge();

    // â”€â”€ Confirmation overlay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (showConfirm && humanVoteTarget >= 0) {
        sf::RectangleShape dim({1280.f, 720.f});
        dim.setFillColor(sf::Color(0,0,0,170));
        window.draw(dim);

        sf::RectangleShape box({480.f, 180.f});
        box.setPosition({400.f, 270.f});
        box.setFillColor(sf::Color(25,25,40));
        box.setOutlineColor(sf::Color(120,120,180));
        box.setOutlineThickness(2.f);
        window.draw(box);

        drawCentered("Vote to eliminate " + players[humanVoteTarget].getName() + "?",
                     22, sf::Color::White, 290.f);

        sf::FloatRect yes({420.f, 360.f}, {160.f, 46.f});
        sf::FloatRect no( {600.f, 360.f}, {160.f, 46.f});
        drawButton(yes, "YES", sf::Color(130,20,20));
        drawButton(no,  "NO",  sf::Color(40,60,40));

        if (btnClicked(yes)) {
            voteCounts[humanVoteTarget]++;
            roundVotes[humanIdx] = humanVoteTarget;
            humanVoted  = true;
            showConfirm = false;
            playSfx(sfxConfirm.get(), 44.f, true);
        }
        if (btnClicked(no)) {
            showConfirm     = false;
            humanVoteTarget = -1;
        }
    }

    // â”€â”€ Elimination result overlay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (showElim) {
        sf::RectangleShape dim({1280.f, 720.f});
        dim.setFillColor(sf::Color(0,0,0,140));
        window.draw(dim);

        if (tallyAnim) {
            float holdT = elimClock.getElapsedTime().asSeconds();
            float p = 0.5f + 0.5f * std::sin(holdT * 6.f);
            drawCentered("FINAL TALLY...", uiFont(34), sf::Color(220, 175, 90, static_cast<std::uint8_t>(170 + p * 80.f)), uiY(0.40f));
            drawCentered("The verdict is about to drop", uiFont(18), sf::Color(180, 170, 150), uiY(0.47f));
            return;
        }

        if (eliminatedIdx >= 0) {
            float overlayY = uiY(0.33f);
            float resultPulse = 0.5f + 0.5f * std::sin(uiClock.getElapsedTime().asSeconds() * 6.f);

            sf::RectangleShape verdictGlow({uiX(0.8f), uiY(0.18f)});
            verdictGlow.setPosition({uiX(0.1f), uiY(0.285f)});
            verdictGlow.setFillColor(sf::Color(40, 8, 8, 70));
            verdictGlow.setOutlineColor(sf::Color(255, 80, 80, static_cast<std::uint8_t>(110 + resultPulse * 100.f)));
            verdictGlow.setOutlineThickness(3.f);
            window.draw(verdictGlow);

            sf::Text elimA(font,
                           wrapTextToWidth(font, uiFont(28), players[eliminatedIdx].getName() + " HAS BEEN ELIMINATED", uiX(0.75f)),
                           uiFont(28));
            elimA.setFillColor(sf::Color(255,80,80, static_cast<std::uint8_t>(220 + resultPulse * 35.f)));
            auto eab = elimA.getLocalBounds();
            elimA.setOrigin({eab.position.x + eab.size.x * 0.5f, eab.position.y});
            elimA.setPosition({uiX(0.5f), overlayY});
            snapTextToPixel(elimA);
            window.draw(elimA);
            overlayY += elimA.getGlobalBounds().size.y + uiY(0.018f);

            sf::Text elimB(font,
                           wrapTextToWidth(font, uiFont(24), players[eliminatedIdx].getName() + " WAS A " + players[eliminatedIdx].getRoleName() + "!", uiX(0.75f)),
                           uiFont(24));
            elimB.setFillColor(sf::Color(255,220,80));
            auto ebb = elimB.getLocalBounds();
            elimB.setOrigin({ebb.position.x + ebb.size.x * 0.5f, ebb.position.y});
            elimB.setPosition({uiX(0.5f), overlayY});
            snapTextToPixel(elimB);
            window.draw(elimB);
            overlayY += elimB.getGlobalBounds().size.y + uiY(0.014f);
            // Show last will
            if (eliminatedIdx < (int)lastWills.size() &&
                !lastWills[eliminatedIdx].empty()) {
                sf::Text wHdr(font, "LAST WILL:", uiFont(16));
                wHdr.setFillColor(sf::Color(200, 200, 140));
                auto whb = wHdr.getLocalBounds();
                wHdr.setOrigin({whb.position.x + whb.size.x * 0.5f, whb.position.y});
                wHdr.setPosition({uiX(0.5f), overlayY});
                snapTextToPixel(wHdr);
                window.draw(wHdr);
                overlayY += wHdr.getGlobalBounds().size.y + uiY(0.01f);

                sf::Text wBody(font,
                               wrapTextToWidth(font, uiFont(14), "\"" + lastWills[eliminatedIdx] + "\"", uiX(0.72f)),
                               uiFont(14));
                wBody.setFillColor(sf::Color(180, 180, 160));
                auto wbb = wBody.getLocalBounds();
                wBody.setOrigin({wbb.position.x + wbb.size.x * 0.5f, wbb.position.y});
                wBody.setPosition({uiX(0.5f), overlayY});
                snapTextToPixel(wBody);
                window.draw(wBody);
            }
        } else {
            drawCentered("NO ONE WAS ELIMINATED (TIE)", uiFont(28), sf::Color(160,160,160), uiY(0.40f));
        }

        if (elimContinueReady) {
            sf::FloatRect cont({490.f, 390.f}, {300.f, 50.f});
            drawButton(cont, "Continue", sf::Color(40,60,40));
            if (btnClicked(cont)) {
                // Check joker win
                if (eliminatedIdx >= 0 &&
                    players[eliminatedIdx].getRole() == Player::Role::JOKER) {
                    endGame("Joker", true);
                    return;
                }
                if (!checkWinCondition()) transitionTo(Phase::NIGHT);
            }
        }
    }

    // If human hasn't voted yet, show hint
    if (!humanVoted && !showConfirm && !showElim) {
        drawCentered("Click a player card to vote", 18, sf::Color(180,180,100), safe.position.y + safe.size.y - 20.f);
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  renderGameOver
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderGameOver()
{
    sf::Color factionAccent = kThemeAccentBlue;
    if (winFaction == "Mafia") factionAccent = kThemeDangerRed;
    else if (winFaction == "Joker") factionAccent = kThemeTextSecondary;

    drawBackdrop(kThemeBackground, withAlpha(factionAccent, 34));
    drawPanel(sf::FloatRect({160.f, 68.f}, {960.f, 586.f}), withAlpha(kThemeCard, 232), withAlpha(factionAccent, 120));

    if (!fontLoaded) return;

    float endT = phaseAnimClock.getElapsedTime().asSeconds();

    // Win title
    std::string title = winFaction + " WINS!";
    sf::Color titleCol = factionAccent;
    drawCentered(title, 64, titleCol, 30.f);

    // Human result
    Player& h = players[humanIdx];
    bool humanWon = false;
    if (winFaction == "Mafia"  && (h.getRole()==Player::Role::MAFIA ||
                                    h.getRole()==Player::Role::GODFATHER ||
                                    h.getRole()==Player::Role::SILENCER)) humanWon=true;
    if (winFaction == "Town"   && h.getRole()!=Player::Role::MAFIA &&
                                   h.getRole()!=Player::Role::GODFATHER &&
                                   h.getRole()!=Player::Role::SILENCER) humanWon=true;
    if (winFaction == "Joker"  && h.getRole()==Player::Role::JOKER)      humanWon=true;

    // Animated result headline (fade + scale) with celebratory glow.
    {
        float reveal = std::clamp(endT / 1.1f, 0.f, 1.f);
        float ease = 1.f - std::pow(1.f - reveal, 3.f);
        float pulse = 1.f + 0.03f * std::sin(uiClock.getElapsedTime().asSeconds() * 5.2f) * ease;
        std::string verdict = humanWon ? "YOU WON!" : "YOU LOST";
        sf::Color verdictCol = humanWon ? kThemeAccentBlue : kThemeDangerRed;

        sf::CircleShape glowA(190.f);
        glowA.setOrigin({190.f, 190.f});
        glowA.setPosition({640.f, 122.f});
        glowA.setFillColor(withAlpha(verdictCol, static_cast<std::uint8_t>(30 + 40 * ease)));
        window.draw(glowA);

        sf::CircleShape glowB(120.f);
        glowB.setOrigin({120.f, 120.f});
        glowB.setPosition({640.f, 122.f});
        glowB.setFillColor(withAlpha(sf::Color::White, static_cast<std::uint8_t>(10 + 20 * ease)));
        window.draw(glowB);

        const unsigned verdictBaseSize = humanWon ? uiFont(60) : uiFont(48);
        const unsigned verdictSize = std::max(10u, static_cast<unsigned>(std::round(verdictBaseSize * pulse)));
        sf::Text verdictText(font, verdict, verdictSize);
        verdictText.setFillColor(withAlpha(verdictCol, static_cast<std::uint8_t>(255 * ease)));
        auto vb = verdictText.getLocalBounds();
        verdictText.setOrigin({vb.position.x + vb.size.x * 0.5f, vb.position.y + vb.size.y * 0.5f});
        verdictText.setPosition({640.f, 126.f});
        snapTextToPixel(verdictText);
        window.draw(verdictText);

        // Lightweight sparkles around the verdict for visual excitement.
        for (int i = 0; i < 10; ++i) {
            float a = uiClock.getElapsedTime().asSeconds() * (1.1f + i * 0.08f) + i * 0.6f;
            float r = 130.f + (i % 3) * 26.f + std::sin(a * 1.7f) * 8.f;
            float x = 640.f + std::cos(a) * r;
            float y = 124.f + std::sin(a) * (34.f + (i % 4) * 8.f);
            sf::CircleShape spark(2.2f + (i % 2));
            spark.setPosition({x, y});
            spark.setFillColor(withAlpha(sf::Color(255, 230, 170), static_cast<std::uint8_t>(80 + 70 * (0.5f + 0.5f * std::sin(a * 2.1f)))));
            window.draw(spark);
        }
    }

    // Role reveal table header
    {
        sf::Text hdr(font, "NAME", 16); hdr.setFillColor(kThemeTextSecondary);
        placeTextTopLeft(hdr, 240.f, 215.f); window.draw(hdr);
        sf::Text hdr2(font, "ROLE", 16); hdr2.setFillColor(kThemeTextSecondary);
        placeTextTopLeft(hdr2, 500.f, 215.f); window.draw(hdr2);
        sf::Text hdr3(font, "STATUS", 16); hdr3.setFillColor(kThemeTextSecondary);
        placeTextTopLeft(hdr3, 740.f, 215.f); window.draw(hdr3);
    }

    float ry = 240.f;
    for (int i = 0; i < (int)players.size(); ++i) {
        bool alive = players[i].getIsAlive();
        sf::RectangleShape rowBg({700.f, 20.f});
        rowBg.setPosition({228.f, ry - 2.f});
        rowBg.setFillColor(alive ? sf::Color(22, 44, 34, 130) : sf::Color(40, 20, 22, 95));
        rowBg.setOutlineColor(alive ? sf::Color(100, 220, 150, 90) : sf::Color(170, 80, 80, 60));
        rowBg.setOutlineThickness(1.f);
        window.draw(rowBg);

        sf::Text nm(font, players[i].getName(), 15);
        nm.setFillColor(alive ? sf::Color(240, 255, 245) : kThemeTextPrimary);
        placeTextTopLeft(nm, 240.f, ry); window.draw(nm);

        sf::Text rl(font, players[i].getRoleName(), 15);
        rl.setFillColor(kThemeTextSecondary); placeTextTopLeft(rl, 500.f, ry); window.draw(rl);

        sf::RectangleShape statusPill({92.f, 18.f});
        statusPill.setPosition({734.f, ry - 1.f});
        statusPill.setFillColor(alive ? sf::Color(22, 80, 48, 170) : sf::Color(90, 28, 28, 160));
        statusPill.setOutlineColor(alive ? sf::Color(120, 235, 170, 130) : sf::Color(220, 95, 95, 120));
        statusPill.setOutlineThickness(1.f);
        window.draw(statusPill);

        sf::Text st(font, alive ? "ALIVE" : "DEAD", 14);
        st.setFillColor(alive ? sf::Color(150, 245, 190) : sf::Color(255, 150, 150));
        auto sb = st.getLocalBounds();
        placeTextTopLeft(st, 780.f - sb.size.x * 0.5f, ry - 1.f);
        window.draw(st);

        ry += 22.f;
    }

    sf::FloatRect playAgain({240.f, 650.f}, {220.f, 48.f});
    sf::FloatRect mainMenu ({500.f, 650.f}, {220.f, 48.f});
    drawButton(playAgain, "PLAY AGAIN",  kThemeAccentBlue);
    drawButton(mainMenu,  "MAIN MENU",   kThemeDangerRed);

    if (btnClicked(playAgain)) {
        // Keep lobby settings, re-assign roles with same human name
        std::string savedName = humanName;
        transitionTo(Phase::LOBBY);
        humanName = savedName;
    }
    if (btnClicked(mainMenu)) { transitionTo(Phase::MAIN_MENU); }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Night logic
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::resolveNight()
{
    Player& h   = players[humanIdx];
    Player::Role hr = h.getRole();

    // Determine mafia kill target
    int mafiaKill = -1;
    if (hr == Player::Role::MAFIA || hr == Player::Role::GODFATHER) {
        mafiaKill = nightHumanTarget;
    }
    // AI mafia
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || players[i].getIsHuman()) continue;
        if (players[i].getRole()==Player::Role::MAFIA ||
            players[i].getRole()==Player::Role::GODFATHER) {
            if (mafiaKill == -1) mafiaKill = aiMafiaTarget();
        }
    }

    // Determine doctor save
    int doctorSave = -1;
    if (hr == Player::Role::DOCTOR) {
        doctorSave = nightHumanTarget;
    }
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || players[i].getIsHuman()) continue;
        if (players[i].getRole() == Player::Role::DOCTOR) {
            if (doctorSave == -1) doctorSave = aiDoctorSave(mafiaKill);
        }
    }
    if (doctorSave >= 0) lastDoctorSavedTarget = doctorSave;

    // AI detective (GodFather appears as Villager)
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || players[i].getIsHuman()) continue;
        if (players[i].getRole() == Player::Role::DETECTIVE) {
            int t = aiDetectiveTarget();
            if (t >= 0) aiInvestigated.push_back(players[t].getName());
        }
    }

    // AI silencer: pick a random non-mafia alive player to silence
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || players[i].getIsHuman()) continue;
        if (players[i].getRole() == Player::Role::SILENCER) {
            std::vector<int> targets;
            for (int j = 0; j < (int)players.size(); ++j) {
                if (players[j].getIsAlive() && j != i &&
                    !isMafiaAlignedRole(players[j].getRole()))
                    targets.push_back(j);
            }
            if (!targets.empty()) {
                int t = targets[rand() % targets.size()];
                players[t].silence();
                silencedPlayerIdx = t;
            }
        }
    }

    // Human silencer action
    if (hr == Player::Role::SILENCER && silencerTarget >= 0 &&
        silencerTarget < (int)players.size() && players[silencerTarget].getIsAlive() &&
        !isMafiaAlignedRole(players[silencerTarget].getRole())) {
        players[silencerTarget].silence();
        silencedPlayerIdx = silencerTarget;
    }

    // Resolve kill vs save
    std::string dayAnnouncement;
    if (mafiaKill >= 0 && mafiaKill != doctorSave) {
        players[mafiaKill].die();
        lastKilledIdx = mafiaKill;
        killLog.push_back({roundNumber, players[mafiaKill].getName(), "Killed by Mafia"});
        // Generate last will for killed player
        if (!players[mafiaKill].getIsHuman() &&
            mafiaKill < (int)lastWills.size() && lastWills[mafiaKill].empty()) {
            lastWills[mafiaKill] = generateAiLastWill(mafiaKill);
        }
        triggerShake(12.f);
        spawnParticles({640.f, 360.f}, sf::Color(200,30,30), 40);
        playSfx(sfxDeath.get(), 64.f);
        dayAnnouncement = players[mafiaKill].getName() + " WAS KILLED";
    } else if (mafiaKill >= 0) {
        lastKilledIdx = -1;
        dayAnnouncement = "NO ONE DIED\nThe doctor saved someone!";
    } else {
        lastKilledIdx = -1;
        dayAnnouncement = "NO ONE DIED";
    }
    roundNumber++;

    // Parity can shift after a night kill; end immediately when a faction has won.
    if (checkWinCondition()) return;

    transitionTo(Phase::DAY, dayAnnouncement);
}

int Game::aiMafiaTarget() const
{
    int best = -1;
    float bestScore = -1e9f;
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || isMafiaAlignedPlayer(i)) continue;

        float score = 0.f;
        if (players[i].getRole() == Player::Role::DETECTIVE) score += 65.f;
        if (players[i].getRole() == Player::Role::DOCTOR) score += 42.f;
        if (players[i].getIsHuman()) score += 22.f;

        float pub = (i < (int)publicSuspicion.size()) ? publicSuspicion[i] : 0.f;
        score += std::clamp(24.f - pub, 0.f, 24.f); // kill trusted players first

        auto it = accusations.find(i);
        if (it != accusations.end()) score -= it->second * 2.f; // if likely voted anyway, lower priority

        float cred = (i < (int)publicCredibility.size()) ? publicCredibility[i] : 50.f;
        score += std::clamp(cred - 50.f, -10.f, 10.f) * 0.8f; // trusted players are better hidden kills

        score -= projectedDoctorProtectionRisk(i);

        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

int Game::aiDoctorSave(int predictedKill) const
{
    if (predictedKill >= 0 && rand() % 100 < 75 && predictedKill != lastDoctorSavedTarget)
        return predictedKill;

    int best = -1;
    float bestScore = -1e9f;
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive()) continue;

        float score = 0.f;
        if (players[i].getRole() == Player::Role::DETECTIVE) score += 30.f;
        if (players[i].getIsHuman()) score += 16.f;
        if (isMafiaAlignedPlayer(i)) score -= 30.f;
        if (i == lastDoctorSavedTarget) score -= 24.f; // avoid predictable back-to-back saves

        float pub = (i < (int)publicSuspicion.size()) ? publicSuspicion[i] : 0.f;
        score += std::clamp(20.f - pub, 0.f, 20.f);

        float cred = (i < (int)publicCredibility.size()) ? publicCredibility[i] : 50.f;
        score += std::clamp(cred - 45.f, 0.f, 20.f) * 0.7f;

        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

int Game::aiDetectiveTarget() const
{
    int best = -1;
    float bestScore = -1e9f;
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || players[i].getIsHuman()) continue;

        bool checked = false;
        for (auto& s : aiInvestigated) if (s == players[i].getName()) checked=true;
        if (checked) continue;

        float score = (i < (int)publicSuspicion.size()) ? publicSuspicion[i] : 0.f;
        auto it = accusations.find(i);
        if (it != accusations.end()) score += it->second * 3.f;
        if (players[i].getIsHuman()) score += 8.f;

        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

void Game::initAiSuspicion()
{
    const int n = static_cast<int>(players.size());
    aiSuspicion.assign(n, std::vector<float>(n, 0.f));
    publicSuspicion.assign(n, 0.f);
    publicCredibility.assign(n, 50.f);
    aiLastVoteTarget.assign(n, -1);
    lastDoctorSavedTarget = -1;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            aiSuspicion[i][j] = static_cast<float>(rand() % 6); // tiny uncertainty baseline
            if (isMafiaAlignedPlayer(i) && isMafiaAlignedPlayer(j)) aiSuspicion[i][j] = -35.f;
        }

        if (isMafiaAlignedPlayer(i)) publicCredibility[i] = 42.f;
    }
}

void Game::decayAiSuspicion(float factor)
{
    if (aiSuspicion.empty()) return;
    factor = std::clamp(factor, 0.5f, 0.99f);
    for (auto& row : aiSuspicion)
        for (float& v : row)
            v = std::clamp(v * factor, -60.f, 100.f);

    for (float& v : publicSuspicion)
        v = std::clamp(v * factor, 0.f, 100.f);
}

void Game::applySuspicionDelta(int observer, int target, float delta)
{
    if (observer < 0 || target < 0 || observer >= (int)aiSuspicion.size() ||
        target >= (int)aiSuspicion[observer].size() || observer == target) return;
    aiSuspicion[observer][target] = std::clamp(aiSuspicion[observer][target] + delta, -60.f, 100.f);
}

float Game::suspicionScoreFor(int observer, int target) const
{
    if (observer < 0 || target < 0 || observer >= (int)players.size() || target >= (int)players.size())
        return -1e9f;
    if (!players[target].getIsAlive() || observer == target) return -1e9f;

    float score = 0.f;
    if (target < (int)publicSuspicion.size()) score += publicSuspicion[target] * 1.6f;
    auto it = accusations.find(target);
    if (it != accusations.end()) score += it->second * 5.f;
    if (observer < (int)aiSuspicion.size() && target < (int)aiSuspicion[observer].size())
        score += aiSuspicion[observer][target];

    if (players[target].isSilenced()) score += 4.f;
    if (players[target].getIsHuman()) score += 3.f;

    return score;
}

int Game::pickMostSuspiciousTarget(int observer, bool avoidMafiaTeam) const
{
    int best = -1;
    float bestScore = -1e9f;
    for (int t = 0; t < (int)players.size(); ++t) {
        if (!players[t].getIsAlive() || t == observer) continue;
        if (avoidMafiaTeam && isMafiaAlignedPlayer(t)) continue;

        float s = suspicionScoreFor(observer, t);
        if (s > bestScore) {
            bestScore = s;
            best = t;
        }
    }
    return best;
}

bool Game::isMafiaAlignedPlayer(int idx) const
{
    if (idx < 0 || idx >= (int)players.size()) return false;
    return isMafiaAlignedRole(players[idx].getRole());
}

void Game::registerVoteSuspicionSignals(int eliminatedTarget)
{
    if (players.empty()) return;

    for (const auto& [voter, target] : roundVotes) {
        if (target >= 0 && target < (int)publicSuspicion.size())
            publicSuspicion[target] = std::clamp(publicSuspicion[target] + 0.8f, 0.f, 100.f);

        if (voter >= 0 && voter < (int)publicCredibility.size()) {
            if (eliminatedTarget >= 0 && target == eliminatedTarget) {
                bool elimMafia = isMafiaAlignedPlayer(eliminatedTarget);
                float d = elimMafia ? 7.f : -5.f;
                publicCredibility[voter] = std::clamp(publicCredibility[voter] + d, 0.f, 100.f);
            } else if (target >= 0 && target < (int)players.size() && !players[target].getIsAlive()) {
                publicCredibility[voter] = std::clamp(publicCredibility[voter] - 3.f, 0.f, 100.f);
            }
        }

        for (int observer = 0; observer < (int)players.size(); ++observer) {
            if (!players[observer].getIsAlive() || players[observer].getIsHuman()) continue;
            if (observer == voter || target < 0 || target >= (int)players.size()) continue;

            if (target == observer) applySuspicionDelta(observer, voter, 6.f);

            if (eliminatedTarget >= 0 && eliminatedTarget < (int)players.size() && target == eliminatedTarget) {
                bool elimMafia = isMafiaAlignedPlayer(eliminatedTarget);
                if (elimMafia) applySuspicionDelta(observer, voter, -3.f);
                else applySuspicionDelta(observer, voter, 3.f);
            }
        }
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Voting logic
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::processAiVotes()
{
    voteReveals.clear();
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || players[i].getIsHuman()) continue;
        int t = aiVoteTarget(i);
        if (t >= 0 && t < (int)voteCounts.size()) {
            roundVotes[i] = t;
            if (i >= 0 && i < (int)aiLastVoteTarget.size()) aiLastVoteTarget[i] = t;
            voteReveals.push_back({i, t});
        }
    }
}

int Game::aiVoteTarget(int aiIdx) const
{
    AiPersonality pers = (aiIdx < (int)aiPersonalities.size())
                        ? aiPersonalities[aiIdx] : AiPersonality::OBSERVER;
    bool isMafia = isMafiaAlignedPlayer(aiIdx);

    int bestTarget = -1;
    float bestScore = -1e9f;
    for (int t = 0; t < (int)players.size(); ++t) {
        if (!players[t].getIsAlive() || t == aiIdx) continue;
        if (isMafia && isMafiaAlignedPlayer(t)) continue;

        float score = suspicionScoreFor(aiIdx, t);
        if (isMafia) {
            float pub = (t < (int)publicSuspicion.size()) ? publicSuspicion[t] : 0.f;
            score += pub * 0.8f; // push consensus targets
        }

        float cred = (t < (int)publicCredibility.size()) ? publicCredibility[t] : 50.f;
        score -= std::clamp(cred - 50.f, -20.f, 30.f) * 0.5f;

        int wagon = 0;
        for (const auto& [_, vt] : roundVotes) if (vt == t) wagon++;
        if (pers == AiPersonality::AGGRESSIVE || pers == AiPersonality::PARANOID) score += wagon * 2.8f;
        if (pers == AiPersonality::ANALYTICAL) score += wagon * 1.4f;
        if (pers == AiPersonality::DEFENDER) score -= wagon * 1.0f;

        switch (pers) {
            case AiPersonality::AGGRESSIVE: score += 8.f; break;
            case AiPersonality::PARANOID:   score += 5.f; break;
            case AiPersonality::ANALYTICAL: score += 3.f; break;
            case AiPersonality::DEFENDER:   score -= 8.f; break;
            case AiPersonality::OBSERVER:   score += 0.f; break;
        }
        score += aiAccusationBias * 0.5f;

        if (score > bestScore) {
            bestScore = score;
            bestTarget = t;
        }
    }

    float threshold = 9.f;
    if (pers == AiPersonality::DEFENDER) threshold = 15.f;
    if (pers == AiPersonality::AGGRESSIVE) threshold = 4.f;
    if (bestTarget >= 0 && bestScore >= threshold) return bestTarget;

    std::vector<int> fallback;
    for (int i = 0; i < (int)players.size(); ++i) {
        if (!players[i].getIsAlive() || i == aiIdx) continue;
        if (isMafia && isMafiaAlignedPlayer(i)) continue;
        fallback.push_back(i);
    }
    if (fallback.empty()) return -1;
    return fallback[rand() % fallback.size()];
}

float Game::projectedDoctorProtectionRisk(int target) const
{
    if (target < 0 || target >= (int)players.size()) return 0.f;

    float risk = 0.f;
    if (target == lastDoctorSavedTarget) risk += 18.f;

    if (target < (int)publicCredibility.size()) {
        float cred = publicCredibility[target];
        risk += std::clamp((cred - 55.f) * 0.3f, 0.f, 10.f);
    }

    if (players[target].getIsHuman()) risk += 4.f;
    return risk;
}

int Game::findMajorityVote() const
{
    int maxV = 0, winner = -1, tieCount = 0;
    for (int i = 0; i < (int)voteCounts.size(); ++i) {
        if (voteCounts[i] > maxV) {
            maxV = voteCounts[i]; winner = i; tieCount = 1;
        } else if (voteCounts[i] == maxV && maxV > 0) {
            tieCount++;
        }
    }
    return (tieCount == 1) ? winner : -1;
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  AI Chat (personality-driven)
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•

static const char* kAggressiveChat[] = {
    "I'm telling you, [NAME] is the killer!",
    "VOTE [NAME] NOW or we all die!",
    "[NAME] is 100% mafia, I'm sure of it",
    "Why is nobody voting [NAME]?!",
    "[NAME] killed [DEAD], I'd bet my life on it",
};
static const char* kObserverChat[] = {
    "[NAME] was oddly quiet last round...",
    "Has anyone noticed [NAME]'s behavior?",
    "Interesting that [NAME] survived...",
    "I'm watching [NAME] closely",
    "Something about [NAME] doesn't add up",
};
static const char* kDefenderChat[] = {
    "Hold on, [NAME] might be innocent",
    "Let's not rush to judge [NAME]",
    "I think [NAME] is telling the truth",
    "We should hear [NAME] out first",
    "Don't vote [NAME] without evidence",
};
static const char* kAnalyticalChat[] = {
    "Based on the evidence, [NAME] is suspicious",
    "The vote pattern suggests [NAME] is hiding something",
    "Statistically, [NAME] is the most likely mafia",
    "If [DEAD] was town, then [NAME] had motive",
    "Cross-referencing clues points to [NAME]",
};
static const char* kParanoidChat[] = {
    "I don't trust ANYONE, especially [NAME]",
    "We're all going to die if we don't vote [NAME]",
    "[NAME] is lying, I can feel it",
    "Nobody is safe! [NAME] could be the killer!",
    "I think [NAME] and someone else are working together",
};

void Game::postAiChat()
{
    std::vector<int> aiAlive;
    for (int i = 0; i < (int)players.size(); ++i)
        if (players[i].getIsAlive() && !players[i].getIsHuman() && !players[i].isSilenced())
            aiAlive.push_back(i);
    if (aiAlive.empty()) return;

    int poster = aiAlive[rand() % aiAlive.size()];
    int suspect = pickMostSuspiciousTarget(poster, isMafiaAlignedPlayer(poster));

    if (suspect < 0) return;

    AiPersonality pers = (poster < (int)aiPersonalities.size())
                        ? aiPersonalities[poster] : AiPersonality::OBSERVER;

    std::string msg;
    int susp = static_cast<int>(std::clamp(suspicionScoreFor(poster, suspect), 0.f, 100.f));
    int cred = (suspect >= 0 && suspect < (int)publicCredibility.size())
             ? static_cast<int>(std::clamp(publicCredibility[suspect], 0.f, 100.f)) : 50;
    std::string targetName = players[suspect].getName();

    switch (pers) {
        case AiPersonality::AGGRESSIVE:
            msg = "I'm voting " + targetName + ". This is too suspicious to ignore.";
            break;
        case AiPersonality::OBSERVER:
            msg = targetName + " keeps drawing attention. Credibility feels low.";
            break;
        case AiPersonality::DEFENDER: {
            int defend = -1;
            float minSusp = 1e9f;
            for (int i = 0; i < (int)players.size(); ++i) {
                if (!players[i].getIsAlive() || i == poster) continue;
                float s = suspicionScoreFor(poster, i);
                if (s < minSusp) { minSusp = s; defend = i; }
            }
            if (defend >= 0) {
                msg = "I don't trust this push on " + players[defend].getName() + ". We need stronger evidence.";
                applySuspicionDelta(poster, defend, -4.f);
                if (defend < (int)publicSuspicion.size())
                    publicSuspicion[defend] = std::clamp(publicSuspicion[defend] - 1.5f, 0.f, 100.f);
                suspect = -1;
            } else {
                msg = "Let's slow down and review who benefited from last night.";
            }
            break;
        }
        case AiPersonality::ANALYTICAL:
            msg = "Current read: " + targetName + " has top suspicion (" + std::to_string(susp)
                + ") and credibility around " + std::to_string(cred) + ".";
            break;
        case AiPersonality::PARANOID:
            msg = "No one is safe, but " + targetName + " worries me the most right now.";
            break;
    }

    {
        std::lock_guard<std::mutex> lock(chatMutex);
        chat.push_back({ players[poster].getName(), msg, kColors[poster % 10] });
    }
    playSfx(sfxChatOpen.get(), 18.f, true);

    if (suspect >= 0) {
        accusations[suspect]++;
        if (suspect < (int)publicSuspicion.size())
            publicSuspicion[suspect] = std::clamp(publicSuspicion[suspect] + 2.8f, 0.f, 100.f);

        for (int i = 0; i < (int)players.size(); ++i) {
            if (!players[i].getIsAlive() || i == poster || i == suspect) continue;
            if (isMafiaAlignedPlayer(poster) && isMafiaAlignedPlayer(i)) continue;
            applySuspicionDelta(i, suspect, 2.f);
        }
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Win condition
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
bool Game::checkWinCondition()
{
    int aliveMafia = 0, aliveNonMafia = 0;
    for (auto& p : players) {
        if (!p.getIsAlive()) continue;
        if (p.getRole()==Player::Role::MAFIA || p.getRole()==Player::Role::GODFATHER
            || p.getRole()==Player::Role::SILENCER)
            aliveMafia++;
        else
            aliveNonMafia++;
    }

    if (aliveMafia == 0) {
        endGame("Town");  return true;
    }
    if (aliveMafia >= aliveNonMafia) {
        endGame("Mafia"); return true;
    }
    return false;
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Transition system
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::transitionTo(Phase target, const std::string& args)
{
    transitioning = true;
    transTarget   = target;
    transArgs     = args;
    transSwapped  = false;
    transClock.restart();
}

void Game::applyTransitionTarget()
{
    switch (transTarget) {
        case Phase::NIGHT:       startNight(); break;
        case Phase::DAY:         startDay(transArgs); break;
        case Phase::DISCUSSION:  startDiscussion(); break;
        case Phase::VOTING:      startVoting(); break;
        case Phase::ROLE_REVEAL: startRoleReveal(); break;
        case Phase::GAME_OVER:   endGame(transArgs); break;
        case Phase::MAIN_MENU:   startMainMenu(); break;
        case Phase::SETTINGS:    startSettings(); break;
        case Phase::INSTRUCTIONS:startInstructions(); break;
        case Phase::LOBBY:       startLobby(); break;
        default:                 startMainMenu(); break;
    }
}

void Game::updateTransition()
{
    float t = transClock.getElapsedTime().asSeconds();
    if (t >= TRANS_DUR / 2.f && !transSwapped) {
        transSwapped = true;
        applyTransitionTarget();
    }
    if (t >= TRANS_DUR) {
        transitioning = false;
    }
}

void Game::renderTransitionOverlay()
{
    if (!transitioning) return;
    float t = transClock.getElapsedTime().asSeconds();
    float half = TRANS_DUR / 2.f;
    float alpha;
    if (t < half) alpha = t / half;
    else          alpha = 1.f - (t - half) / half;
    alpha = std::clamp(alpha, 0.f, 1.f);
    alpha = alpha * alpha * (3.f - 2.f * alpha); // smoothstep easing

    sf::RectangleShape overlay({1280.f, 720.f});
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(alpha * 255.f)));
    window.draw(overlay);
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Particle system
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::triggerShake(float intensity)
{
    shakeIntensity = intensity;
    shakeClock.restart();
}

void Game::spawnParticles(sf::Vector2f pos, sf::Color col, int count, bool confetti)
{
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = pos;
        float angle = (rand() % 360) * 3.14159f / 180.f;
        float speed = 50.f + rand() % 200;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed - (confetti ? 150.f : 0.f)};
        if (confetti) {
            p.color = kColors[rand() % 10];
        } else {
            p.color = col;
        }
        p.maxLife = 1.0f + (rand() % 10) / 10.f;
        p.life = p.maxLife;
        particles.push_back(p);
    }
}

void Game::updateParticles(float dt)
{
    for (auto& p : particles) {
        p.pos += p.vel * dt;
        p.vel.y += 200.f * dt; // gravity
        p.life -= dt;
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
                        [](const Particle& p) { return p.life <= 0.f; }),
        particles.end());
}

void Game::renderParticles()
{
    for (auto& p : particles) {
        float alpha = std::clamp(p.life / p.maxLife, 0.f, 1.f);
        float sz = 2.f + alpha * 3.f;
        sf::CircleShape c(sz);
        c.setPosition({p.pos.x - sz, p.pos.y - sz});
        c.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b,
                                  static_cast<uint8_t>(alpha * 255.f)));
        window.draw(c);
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Crewmate drawing
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::drawCrewmate(float cx, float cy, float s, sf::Color bodyCol, bool dead)
{
    uint8_t a = dead ? 80 : 255;
    sf::Color bc(bodyCol.r, bodyCol.g, bodyCol.b, a);

    // Body (rounded rectangle approximation)
    sf::RectangleShape body({16.f * s, 22.f * s});
    body.setPosition({cx - 8.f * s, cy - 11.f * s});
    body.setFillColor(bc);
    window.draw(body);

    // Head dome
    sf::CircleShape head(8.f * s);
    head.setPosition({cx - 8.f * s, cy - 24.f * s});
    head.setFillColor(bc);
    window.draw(head);

    // Visor
    sf::RectangleShape visor({10.f * s, 6.f * s});
    visor.setPosition({cx - 2.f * s, cy - 18.f * s});
    visor.setFillColor(dead ? sf::Color(60, 60, 80, a) : sf::Color(180, 220, 255, a));
    window.draw(visor);

    // Backpack
    sf::RectangleShape backpack({5.f * s, 12.f * s});
    backpack.setPosition({cx - 13.f * s, cy - 6.f * s});
    backpack.setFillColor(bc);
    window.draw(backpack);

    if (!dead) {
        // Legs
        sf::RectangleShape legL({6.f * s, 6.f * s});
        legL.setPosition({cx - 7.f * s, cy + 11.f * s});
        legL.setFillColor(bc);
        window.draw(legL);
        sf::RectangleShape legR({6.f * s, 6.f * s});
        legR.setPosition({cx + 1.f * s, cy + 11.f * s});
        legR.setFillColor(bc);
        window.draw(legR);
    } else {
        // Ghost "X" eyes
        if (fontLoaded) {
            sf::Text x(font, "x", static_cast<unsigned>(8.f * s));
            x.setFillColor(sf::Color(200,60,60, a));
            x.setPosition({cx - 1.f * s, cy - 20.f * s});
            window.draw(x);
        }
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Round badge
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::drawRoundBadge()
{
    if (!fontLoaded) return;
    std::string phaseName;
    switch (phase) {
        case Phase::NIGHT:      phaseName = "Night"; break;
        case Phase::DAY:        phaseName = "Day"; break;
        case Phase::DISCUSSION: phaseName = "Discussion"; break;
        case Phase::VOTING:     phaseName = "Vote"; break;
        default: return;
    }
    std::string badge = phaseName + " " + std::to_string(roundNumber);
    sf::Text t(font, badge, 14);
    t.setFillColor(sf::Color(140,140,180));
    t.setPosition({1180.f, 10.f});
    window.draw(t);
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Evidence system
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::generateEvidence()
{
    // Generate 2-3 truthful evidence per round
    int numEvidence = 2 + rand() % 2;
    std::vector<int> alive;
    for (int i = 0; i < (int)players.size(); ++i)
        if (players[i].getIsAlive()) alive.push_back(i);
    if (alive.empty()) return;

    for (int e = 0; e < numEvidence; ++e) {
        int type = rand() % 4;
        int p1 = alive[rand() % alive.size()];

        if (type == 0) {
            // Location sighting
            std::string loc = kLocations[rand() % kLocationCount];
            evidence.push_back({players[p1].getName() + " was seen near " + loc, roundNumber});
        } else if (type == 1) {
            // Suspicious absence (true for mafia-aligned)
            bool isMafia = (players[p1].getRole() == Player::Role::MAFIA ||
                           players[p1].getRole() == Player::Role::GODFATHER ||
                           players[p1].getRole() == Player::Role::SILENCER);
            if (isMafia)
                evidence.push_back({players[p1].getName() + " was suspiciously absent last night", roundNumber});
            else
                evidence.push_back({players[p1].getName() + " was seen sleeping peacefully", roundNumber});
        } else if (type == 2 && alive.size() >= 2) {
            // Alibi confirmed (both innocent)
            for (int p2 : alive) {
                if (p2 != p1 &&
                    players[p1].getRole() != Player::Role::MAFIA &&
                    players[p2].getRole() != Player::Role::MAFIA) {
                    evidence.push_back({players[p1].getName() + "'s alibi was confirmed by "
                                       + players[p2].getName(), roundNumber});
                    break;
                }
            }
        } else {
            // Night action count
            int actionCount = 0;
            for (auto& pl : players)
                if (pl.getIsAlive() && pl.getRole() != Player::Role::VILLAGER && pl.getRole() != Player::Role::JOKER)
                    actionCount++;
            evidence.push_back({std::to_string(actionCount) + " players were active during the night", roundNumber});
        }
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Journal (TAB panel)
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderJournal()
{
    if (!fontLoaded) return;

    // Semi-transparent panel on right
    sf::FloatRect journalRect = uiRect(0.688f, 0.056f, 0.297f, 0.889f);
    sf::RectangleShape bg({journalRect.size.x, journalRect.size.y});
    bg.setPosition({journalRect.position.x, journalRect.position.y});
    bg.setFillColor(sf::Color(15, 15, 25, 230));
    bg.setOutlineColor(sf::Color(60, 80, 140));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    const sf::FloatRect inner = insetRect(journalRect);
    float y = inner.position.y;
    const float x = inner.position.x;
    const float textMaxWidth = inner.size.x;
    const float lineSpacing = uiY(0.006f);
    const float sectionSpacing = kUiSectionSpacingPx * 0.25f;
    const float bottomLimit = inner.position.y + inner.size.y - kUiContainerPaddingPx;

    auto drawWrappedAt = [&](const std::string& text, unsigned int size, sf::Color col, float x, float& yCursor) {
        sf::Text t(font, wrapTextToWidth(font, size, text, textMaxWidth), size);
        t.setFillColor(col);
        t.setPosition({x, yCursor});
        window.draw(t);
        yCursor += t.getGlobalBounds().size.y + lineSpacing;
    };

    sf::Text title(font, "JOURNAL (TAB to close)", uiFont(16));
    title.setFillColor(sf::Color(180, 200, 255));
    title.setPosition({x, y});
    window.draw(title);
    y += title.getGlobalBounds().size.y + sectionSpacing;

    // Detective clues
    if (!clues.empty()) {
        sf::Text hdr(font, "-- Detective Clues --", uiFont(13));
        hdr.setFillColor(sf::Color(100, 160, 255));
        hdr.setPosition({x, y});
        window.draw(hdr);
        y += hdr.getGlobalBounds().size.y + sectionSpacing;
        for (auto& [nm, rl] : clues) {
            Player tmp; tmp.setRole(rl);
            drawWrappedAt(nm + ": " + tmp.getRoleName(), uiFont(12), sf::Color(200, 220, 255), x, y);
            if (y > bottomLimit) break;
        }
        y += sectionSpacing;
    }

    // Evidence
    if (!evidence.empty()) {
        sf::Text hdr(font, "-- Evidence --", uiFont(13));
        hdr.setFillColor(sf::Color(255, 200, 100));
        hdr.setPosition({x, y});
        window.draw(hdr);
        y += hdr.getGlobalBounds().size.y + sectionSpacing;
        // Show most recent first
        for (int i = (int)evidence.size() - 1; i >= 0 && y < bottomLimit - 50.f; --i) {
            std::string prefix = "[R" + std::to_string(evidence[i].round) + "] ";
            drawWrappedAt(prefix + evidence[i].text, uiFont(11), sf::Color(200, 200, 180), x, y);
        }
        y += sectionSpacing;
    }

    // Kill log
    if (!killLog.empty()) {
        sf::Text hdr(font, "-- Kill Log --", uiFont(13));
        hdr.setFillColor(sf::Color(255, 100, 100));
        hdr.setPosition({x, y});
        window.draw(hdr);
        y += hdr.getGlobalBounds().size.y + sectionSpacing;
        for (auto& k : killLog) {
            if (y > bottomLimit - 18.f) break;
            drawWrappedAt("[R" + std::to_string(k.round) + "] " + k.name + " - " + k.cause,
                          uiFont(11), sf::Color(220, 150, 150), x, y);
        }
        y += sectionSpacing;
    }

    // Vote history
    if (!voteHistory.empty() && y < bottomLimit - 30.f) {
        sf::Text hdr(font, "-- Vote History --", uiFont(13));
        hdr.setFillColor(sf::Color(200, 200, 100));
        hdr.setPosition({x, y});
        window.draw(hdr);
        y += hdr.getGlobalBounds().size.y + sectionSpacing;
        for (auto& vh : voteHistory) {
            if (y > bottomLimit) break;
            std::string elim = vh.eliminated >= 0 ? players[vh.eliminated].getName() : "No one";
            drawWrappedAt("R" + std::to_string(vh.round) + ": eliminated " + elim,
                          uiFont(11), sf::Color(200, 200, 150), x, y);
        }
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Tooltip
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderTooltip()
{
    if (!fontLoaded) return;
    if (phase != Phase::DISCUSSION && phase != Phase::VOTING && phase != Phase::DAY) return;

    // Find card under mouse
    hoveredCardIdx = -1;
    // Simple check: scan grid positions used in current phase
    float gx, gy; int col = 0;
    sf::FloatRect safe = uiSafeArea(kUiSafeMarginPx);
    if (phase == Phase::DAY) { gx = safe.position.x + 20.f; gy = safe.position.y + 140.f; }
    else if (phase == Phase::DISCUSSION) { gx = safe.position.x; gy = safe.position.y + 60.f; }
    else { gx = safe.position.x + 10.f; gy = safe.position.y + 50.f; }
    float startGx = gx;
    int maxCol = (phase == Phase::DISCUSSION) ? 3 : (phase == Phase::VOTING ? 4 : 5);
    float cardW = (phase == Phase::VOTING) ? 220.f : 200.f;
    float cardH = (phase == Phase::VOTING) ? 100.f : 90.f;
    float gap = (phase == Phase::VOTING) ? 240.f : ((phase == Phase::DISCUSSION) ? 210.f : 220.f);
    float gapY = (phase == Phase::VOTING) ? 120.f : ((phase == Phase::DISCUSSION) ? 105.f : 110.f);

    for (int i = 0; i < (int)players.size(); ++i) {
        if (phase != Phase::DAY && !players[i].getIsAlive()) continue;
        sf::FloatRect r({gx, gy}, {cardW, cardH});
        if (r.contains(mousePos)) {
            hoveredCardIdx = i;
            break;
        }
        gx += gap; col++;
        if (col == maxCol) { col = 0; gx = startGx; gy += gapY; }
    }

    if (hoveredCardIdx < 0) return;

    // Build tooltip text
    const Player& p = players[hoveredCardIdx];
    std::string tip = p.getName();
    tip += p.getIsAlive() ? " (Alive)" : " (Dead)";

    // Check if we have clues about this player
    for (auto& [nm, rl] : clues) {
        if (nm == p.getName()) {
            Player tmp; tmp.setRole(rl);
            tip += "\nRole: " + tmp.getRoleName();
            break;
        }
    }

    if (p.isSilenced()) tip += "\nSILENCED";

    // Draw tooltip at mouse
    float tipX = std::min(mousePos.x + 12.f, safe.position.x + safe.size.x - 180.f);
    float tipY = std::min(mousePos.y + 12.f, safe.position.y + safe.size.y - 40.f);

    // Count lines for height
    int lines = 1;
    for (char c : tip) if (c == '\n') lines++;
    float tipH = lines * 16.f + 10.f;
    float tipW = 180.f;

    tipY = std::min(tipY, safe.position.y + safe.size.y - tipH);

    sf::RectangleShape tipBg({tipW, tipH});
    tipBg.setPosition({tipX, tipY});
    tipBg.setFillColor(sf::Color(20, 20, 35, 230));
    tipBg.setOutlineColor(sf::Color(80, 80, 140));
    tipBg.setOutlineThickness(1.f);
    window.draw(tipBg);

    // Split tip by \n and draw each line
    float ly = tipY + 4.f;
    size_t pos = 0;
    while (pos < tip.size()) {
        size_t nl = tip.find('\n', pos);
        if (nl == std::string::npos) nl = tip.size();
        sf::Text t(font, tip.substr(pos, nl - pos), 12);
        t.setFillColor(sf::Color(200, 200, 220));
        t.setPosition({tipX + 6.f, ly});
        window.draw(t);
        ly += 16.f;
        pos = nl + 1;
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Stats save/load
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::loadStats()
{
    std::ifstream f("stats.txt");
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        int val = std::atoi(line.substr(eq+1).c_str());
        if (key == "gamesPlayed") stats.gamesPlayed = val;
        if (key == "wins")        stats.wins = val;
        if (key == "losses")      stats.losses = val;
        if (key == "mafiaWins")   stats.mafiaWins = val;
        if (key == "townWins")    stats.townWins = val;
        if (key == "jokerWins")   stats.jokerWins = val;
        if (key == "nightDurationSec")       nightDurationSec = std::clamp((float)val, 15.f, 60.f);
        if (key == "discussionDurationSec")  discussionDurationSec = std::clamp((float)val, 30.f, 120.f);
        if (key == "aiAccusationBias")       aiAccusationBias = std::clamp(val, -20, 20);
        if (key == "aiChatMinDelaySec")      aiChatMinDelaySec = std::clamp((float)val, 1.f, 12.f);
        if (key == "aiChatMaxDelaySec")      aiChatMaxDelaySec = std::clamp((float)val, 1.f, 12.f);
        if (key == "masterVolume")           masterVolume = std::clamp((float)val, 0.f, 100.f);
        if (key == "uiVolume")               uiVolume = std::clamp((float)val, 0.f, 100.f);
        if (key == "sfxVolume")              sfxVolume = std::clamp((float)val, 0.f, 100.f);
        if (key == "ambientVolume")          ambientVolume = std::clamp((float)val, 0.f, 100.f);
    }

    if (aiChatMaxDelaySec < aiChatMinDelaySec) std::swap(aiChatMaxDelaySec, aiChatMinDelaySec);
}

void Game::saveStats()
{
    std::ofstream f("stats.txt");
    if (!f.is_open()) return;
    f << "gamesPlayed=" << stats.gamesPlayed << "\n";
    f << "wins=" << stats.wins << "\n";
    f << "losses=" << stats.losses << "\n";
    f << "mafiaWins=" << stats.mafiaWins << "\n";
    f << "townWins=" << stats.townWins << "\n";
    f << "jokerWins=" << stats.jokerWins << "\n";
    f << "nightDurationSec=" << (int)nightDurationSec << "\n";
    f << "discussionDurationSec=" << (int)discussionDurationSec << "\n";
    f << "aiAccusationBias=" << aiAccusationBias << "\n";
    f << "aiChatMinDelaySec=" << (int)aiChatMinDelaySec << "\n";
    f << "aiChatMaxDelaySec=" << (int)aiChatMaxDelaySec << "\n";
    f << "masterVolume=" << (int)masterVolume << "\n";
    f << "uiVolume=" << (int)uiVolume << "\n";
    f << "sfxVolume=" << (int)sfxVolume << "\n";
    f << "ambientVolume=" << (int)ambientVolume << "\n";
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Night Sky Animation
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::initNightSky()
{
    nightStars.clear();
    for (int i = 0; i < 150; ++i) {
        Star s;
        s.x = static_cast<float>(rand() % 1280);
        s.y = static_cast<float>(rand() % 720);
        s.baseBright = 0.3f + (rand() % 70) / 100.f; // 0.3 to 1.0
        s.speed = 1.0f + (rand() % 20) / 10.f; // 1.0 to 3.0
        s.phase = static_cast<float>(rand() % 100);
        nightStars.push_back(s);
    }
    
    nightClouds.clear();
    for (int i = 0; i < 5; ++i) {
        NightCloud c;
        c.x = static_cast<float>(rand() % 1280);
        c.y = static_cast<float>(rand() % 400);
        c.w = 200.f + static_cast<float>(rand() % 300);
        c.speed = 5.0f + static_cast<float>(rand() % 15);
        c.alpha = 20.f + static_cast<float>(rand() % 30);
        nightClouds.push_back(c);
    }
    nightSkyInit = true;
}

void Game::renderNightSky()
{
    if (!nightSkyInit) initNightSky();
    
    float t = nightClock.getElapsedTime().asSeconds();
    float dt = frameClock.getElapsedTime().asSeconds(); // somewhat approximate but ok for BG rendering

    // Draw Moon
    sf::CircleShape moon(60.f);
    moon.setPosition({1000.f, 80.f});
    moon.setFillColor(sf::Color(220, 220, 240));
    window.draw(moon);

    // Subtle moon glow
    sf::CircleShape moonGlow(120.f);
    moonGlow.setPosition({940.f, 20.f});
    moonGlow.setFillColor(sf::Color(200, 200, 255, 15));
    window.draw(moonGlow);

    // Draw Stars
    for (auto& s : nightStars) {
        float bright = s.baseBright * (0.5f + 0.5f * std::sin(s.speed * t + s.phase));
        float driftX = std::sin(t * 0.05f + s.phase) * 6.f;
        float driftY = std::cos(t * 0.035f + s.phase) * 1.5f;
        sf::CircleShape star(std::max(1.f, bright * 2.5f));
        star.setPosition({s.x + driftX, s.y + driftY});
        star.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(bright * 255)));
        window.draw(star);
    }

    // Draw Clouds
    for (auto& c : nightClouds) {
        c.x -= c.speed * dt;
        if (c.x + c.w < 0) {
            c.x = 1280.f;
            c.y = static_cast<float>(rand() % 400);
        }
        sf::RectangleShape cloud({c.w, 80.f});
        cloud.setPosition({c.x, c.y});
        cloud.setFillColor(sf::Color(80, 80, 120, static_cast<std::uint8_t>(c.alpha)));
        window.draw(cloud);
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Graveyard
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderGraveyard()
{
    if (!fontLoaded) return;

    sf::RectangleShape overlay({1280.f, 720.f});
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);

    sf::RectangleShape panel({900.f, 500.f});
    panel.setPosition({190.f, 110.f});
    panel.setFillColor(sf::Color(20, 25, 30));
    panel.setOutlineColor(sf::Color(80, 80, 100));
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    drawCentered("GRAVEYARD (G to close)", 30, sf::Color(180, 180, 200), 130.f);

    float gy = 200.f;
    float gx = 250.f;
    int deadCount = 0;

    for (int i = 0; i < (int)players.size(); ++i) {
        if (players[i].getIsAlive()) continue;
        
        deadCount++;
        
        sf::RectangleShape tomb({180.f, 220.f});
        tomb.setPosition({gx, gy});
        tomb.setFillColor(sf::Color(40, 45, 55));
        tomb.setOutlineColor(sf::Color(100, 100, 120));
        tomb.setOutlineThickness(2.f);
        window.draw(tomb);

        // Cross
        sf::RectangleShape vert({12.f, 60.f}), horiz({40.f, 12.f});
        vert.setPosition({gx + 84.f, gy + 20.f});
        horiz.setPosition({gx + 70.f, gy + 35.f});
        vert.setFillColor(sf::Color(80, 85, 95));
        horiz.setFillColor(sf::Color(80, 85, 95));
        window.draw(vert);
        window.draw(horiz);

        sf::Text nm(font, players[i].getName(), 16);
        nm.setFillColor(sf::Color::White);
        fitTextToWidth(nm, 156.f, 10);
        auto b = nm.getLocalBounds();
        nm.setPosition({gx + 90.f - b.size.x/2.f - b.position.x, gy + 100.f});
        snapTextToPixel(nm);
        window.draw(nm);

        sf::Text rl(font, players[i].getRoleName(), 14);
        rl.setFillColor(roleCardColor(players[i].getRole()));
        fitTextToWidth(rl, 156.f, 10);
        auto rb = rl.getLocalBounds();
        rl.setPosition({gx + 90.f - rb.size.x/2.f - rb.position.x, gy + 130.f});
        snapTextToPixel(rl);
        window.draw(rl);

        // Find cause of death
        std::string cause = "Unknown";
        for (auto& k : killLog) {
            if (k.name == players[i].getName()) {
                cause = k.cause + "\n(R" + std::to_string(k.round) + ")";
                break;
            }
        }
        
        sf::Text cs(font, "", 12);
        cs.setFillColor(sf::Color(150, 150, 170));
        applyWrappedTextFitted(cs, font, 12, cause, 156.f, 52.f, 10, 1.08f);
        auto cb = cs.getLocalBounds();
        cs.setPosition({gx + 90.f - cb.size.x/2.f - cb.position.x, gy + 160.f});
        snapTextToPixel(cs);
        window.draw(cs);

        gx += 200.f;
        if (gx > 850.f) { gx = 250.f; gy += 240.f; }
    }

    if (deadCount == 0) {
        drawCentered("The graveyard is peaceful... for now.", 20, sf::Color(120, 120, 140), 300.f);
    }
}

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  Last Will System
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
void Game::renderLastWillPopup()
{
    if (!fontLoaded) return;

    sf::RectangleShape overlay({1280.f, 720.f});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);

    sf::FloatRect panelRect({340.f, 185.f}, {600.f, 350.f});
    sf::RectangleShape panel(panelRect.size);
    panel.setPosition(panelRect.position);
    panel.setFillColor(sf::Color(240, 235, 210)); // Parchment color
    panel.setOutlineColor(sf::Color(140, 120, 80));
    panel.setOutlineThickness(4.f);
    window.draw(panel);

    const sf::FloatRect inner = insetRect(panelRect, 18.f);

    sf::Text hdr(font, "YOUR LAST WILL (W to save & close)", 18);
    hdr.setFillColor(sf::Color(60, 40, 20));
    auto hb = hdr.getLocalBounds();
    const float hdrX = panelRect.position.x + panelRect.size.x * 0.5f - hb.size.x/2.f - hb.position.x;
    const float hdrY = inner.position.y;
    hdr.setPosition({hdrX, hdrY});
    snapTextToPixel(hdr);
    window.draw(hdr);

    sf::Text sub(font, "", 14);
    sub.setFillColor(sf::Color(100, 80, 50));
    applyWrappedText(sub, font, 14, "This will be revealed to the town if you are eliminated.", inner.size.x);
    auto sb = sub.getLocalBounds();
    const auto hdrGlobal = hdr.getGlobalBounds();
    const float subY = std::round(hdrGlobal.position.y + hdrGlobal.size.y + 12.f);
    sub.setPosition({panelRect.position.x + panelRect.size.x * 0.5f - sb.size.x/2.f - sb.position.x, subY});
    snapTextToPixel(sub);
    window.draw(sub);

    // Text box
    sf::FloatRect textBoxRect({inner.position.x + 14.f, inner.position.y + 96.f}, {inner.size.x - 28.f, 150.f});
    sf::RectangleShape tb(textBoxRect.size);
    tb.setPosition(textBoxRect.position);
    tb.setFillColor(sf::Color(255, 252, 235));
    tb.setOutlineColor(sf::Color(180, 160, 120));
    tb.setOutlineThickness(2.f);
    window.draw(tb);

    if (humanIdx >= 0 && humanIdx < (int)lastWills.size()) {
        std::string txt = lastWills[humanIdx] + "|";
        float ty = textBoxRect.position.y + 10.f;
        sf::Text lw(font, "", 16);
        lw.setFillColor(sf::Color(40, 30, 20));

        const std::string wrapped = wrapTextToWidth(font, 16, txt, textBoxRect.size.x - 20.f);
        const float bottomLimit = textBoxRect.position.y + textBoxRect.size.y - 10.f;
        for (const auto& line : splitByNewline(wrapped)) {
            lw.setString(line.empty() ? " " : line);
            if (ty + lw.getLocalBounds().size.y > bottomLimit) break;
            lw.setPosition({textBoxRect.position.x + 10.f, ty});
            window.draw(lw);
            ty += lw.getLocalBounds().size.y + 6.f;
        }
    }
}

std::string Game::generateAiLastWill(int idx)
{
    AiPersonality pers = (idx < (int)aiPersonalities.size())
                        ? aiPersonalities[idx] : AiPersonality::OBSERVER;
    Player::Role role = players[idx].getRole();
    bool isMafia = (role == Player::Role::MAFIA || role == Player::Role::GODFATHER || role == Player::Role::SILENCER);

    // Find who they accused most
    int topSuspect = -1;
    int maxAcc = 0;
    
    // Simplistic: just pick someone currently alive and innocent if mafia, or alive if town
    std::vector<int> candidates;
    for (int i = 0; i < (int)players.size(); ++i) {
        if (i != idx && players[i].getIsAlive()) {
            if (isMafia && players[i].getRole() != Player::Role::MAFIA && players[i].getRole() != Player::Role::GODFATHER) {
                candidates.push_back(i);
            } else if (!isMafia) {
                candidates.push_back(i);
            }
        }
    }
    
    if (!candidates.empty()) {
        topSuspect = candidates[rand() % candidates.size()];
    }

    std::string nm = (topSuspect >= 0) ? players[topSuspect].getName() : "someone";

    if (role == Player::Role::DETECTIVE) {
        if (aiInvestigated.empty()) return "I didn't get to investigate anyone yet...";
        return "I am the Detective. I investigated " + aiInvestigated.back() + " recently.";
    }

    if (isMafia) {
        // Mafia leaves false leads
        const char* falseLeads[] = {
            "I was innocent! Look closely at [NAME], they set me up.",
            "[NAME] is mafia, I swear it.",
            "Town, you made a mistake. Watch [NAME].",
            "I saw [NAME] kill someone.",
            "[NAME] is definitely the killer."
        };
        return replaceAll(falseLeads[rand() % 5], "[NAME]", nm);
    }

    // Town based on personality
    switch (pers) {
        case AiPersonality::AGGRESSIVE:
            return "Avenge me! Vote out " + nm + " immediately!";
        case AiPersonality::ANALYTICAL:
            return "Based on the voting patterns, " + nm + " should be investigated.";
        case AiPersonality::DEFENDER:
            return "Good luck Town. I think " + nm + " is innocent, protect them.";
        case AiPersonality::PARANOID:
            return "Don't trust anyone... especially " + nm + ".";
        default:
            return "I tried my best. Don't let the mafia win. Watch " + nm + ".";
    }
}





