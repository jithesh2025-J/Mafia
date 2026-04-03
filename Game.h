#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <mutex>
#include <memory>
#include "Player.h"
#include "Vote.h"
#include "AIClient.h"

// ─── Top-level game phase ─────────────────────────────────────────────────────
enum class Phase {
    MAIN_MENU, LOBBY, ROLE_REVEAL, NIGHT, DAY, DISCUSSION, VOTING, GAME_OVER, SETTINGS, INSTRUCTIONS
};

// ─── AI personality types ─────────────────────────────────────────────────────
enum class AiPersonality {
    AGGRESSIVE, OBSERVER, DEFENDER, ANALYTICAL, PARANOID
};

class Game
{
public:
    Game();
    ~Game();
    void run();

private:
    // ── Engine ────────────────────────────────────────────────────────────────
    void handleEvents();
    void update();
    void render();
    void updateGameView();
    void initAudio();
    void updateAmbientForPhase();
    void playSfx(sf::Sound* sfx, float volume = 100.f, bool isUi = false);
    void toggleMute();

    // ── Phase transitions ─────────────────────────────────────────────────────
    void startMainMenu();
    void startSettings();
    void startInstructions();
    void startLobby();
    void startRoleReveal();
    void startNight();
    void startDay(const std::string& announcement);
    void startDiscussion();
    void startVoting();
    void endGame(const std::string& faction, bool joker = false);
    void resetGame();
    void assignRoles();
    void transitionTo(Phase target, const std::string& args = "");
    void applyTransitionTarget();

    // ── Phase updates ─────────────────────────────────────────────────────────
    void updateRoleReveal();
    void updateNight();
    void updateDiscussion();
    void updateVoting();
    void updateTransition();
    void updateParticles(float dt);

    // ── Phase renders ─────────────────────────────────────────────────────────
    void renderMainMenu();
    void renderSettings();
    void renderInstructions();
    void renderLobby();
    void renderRoleReveal();
    void renderNight();
    void renderDay();
    void renderDiscussion();
    void renderVoting();
    void renderGameOver();
    void renderTransitionOverlay();
    void renderParticles();
    void renderQuickMenu();
    void renderJournal();
    void renderTooltip();
    void renderNightSky();
    void initNightSky();
    void renderGraveyard();
    void renderLastWillPopup();
    std::string generateAiLastWill(int idx);

    // ── Draw helpers ──────────────────────────────────────────────────────────
    void drawPlayerCard(int idx, float x, float y,
                        bool selected, bool hovered,
                        int voteCount = 0, bool showVotes = false);
    void drawBackdrop(sf::Color base = sf::Color(10, 12, 18),
                      sf::Color accent = sf::Color(80, 100, 180, 32));
    void drawPanel(const sf::FloatRect& r, sf::Color fill,
                   sf::Color outline = sf::Color(110, 120, 160, 120));
    void drawSectionHeader(const std::string& title, const std::string& subtitle,
                           float y, sf::Color accent);
    void drawBadge(const sf::FloatRect& r, const std::string& label,
                   sf::Color fill, sf::Color outline = sf::Color(255, 255, 255, 70));
    void drawTopHud(const std::string& phaseTitle, sf::Color accent);
    void drawButton(const sf::FloatRect& r, const std::string& label,
                    sf::Color fill, sf::Color txtCol = sf::Color::White);
    sf::FloatRect uiSafeArea(float marginPx = 40.f) const;
    sf::FloatRect clampToSafeArea(const sf::FloatRect& r, float insetPx = 0.f) const;
    float uiX(float pct) const;
    float uiY(float pct) const;
    sf::FloatRect uiRect(float xPct, float yPct, float wPct, float hPct) const;
    unsigned int uiFont(unsigned int baseSize) const;
    void drawDebugBounds(const sf::FloatRect& rect, sf::Color color, float thickness = 1.f);
    void drawDebugTextBounds(const sf::Text& text, sf::Color color);
    bool btnClicked(const sf::FloatRect& r);
    bool btnHovered(const sf::FloatRect& r) const;
    void drawCentered(const std::string& s, unsigned sz, sf::Color c, float y);
    void drawProgressBar(float progress, float yPos, float h = 8.f);
    void drawCrewmate(float cx, float cy, float scale, sf::Color bodyCol, bool dead);
    void drawRoundBadge();

    // ── Night logic ───────────────────────────────────────────────────────────
    void resolveNight();
    int  aiMafiaTarget()  const;
    int  aiDoctorSave(int predictedKill) const;
    int  aiDetectiveTarget()  const;

    // ── Voting logic ──────────────────────────────────────────────────────────
    void processAiVotes();
    int  aiVoteTarget(int aiIdx) const;
    int  findMajorityVote()     const;

    // ── AI discussion ─────────────────────────────────────────────────────────
    void postAiChat();
    void initAiSuspicion();
    void decayAiSuspicion(float factor = 0.92f);
    void applySuspicionDelta(int observer, int target, float delta);
    float suspicionScoreFor(int observer, int target) const;
    int pickMostSuspiciousTarget(int observer, bool avoidMafiaTeam) const;
    bool isMafiaAlignedPlayer(int idx) const;
    void registerVoteSuspicionSignals(int eliminatedTarget);
    float projectedDoctorProtectionRisk(int target) const;

    // ── Win condition ─────────────────────────────────────────────────────────
    bool checkWinCondition();

    // ── Evidence ──────────────────────────────────────────────────────────────
    void generateEvidence();

    // ── Effects ───────────────────────────────────────────────────────────────
    void triggerShake(float intensity);
    void spawnParticles(sf::Vector2f pos, sf::Color col, int count, bool confetti = false);

    // ── Assets ────────────────────────────────────────────────────────────────
    sf::Font font;
    sf::Texture bgTexture;
    sf::Sprite bgSprite;
    bool bgLoaded;

    // ── Audio ─────────────────────────────────────────────────────────────────
    bool            audioEnabled = false;
    sf::SoundBuffer sfxClickBuffer;
    sf::SoundBuffer sfxDeathBuffer;
    sf::SoundBuffer sfxVoteTickBuffer;
    sf::SoundBuffer sfxPhaseBuffer;
    sf::SoundBuffer sfxConfirmBuffer;
    sf::SoundBuffer sfxChatOpenBuffer;
    sf::SoundBuffer ambientMenuBuffer;
    sf::SoundBuffer ambientGameBuffer;
    std::unique_ptr<sf::Sound> sfxClick;
    std::unique_ptr<sf::Sound> sfxDeath;
    std::unique_ptr<sf::Sound> sfxVoteTick;
    std::unique_ptr<sf::Sound> sfxPhase;
    std::unique_ptr<sf::Sound> sfxConfirm;
    std::unique_ptr<sf::Sound> sfxChatOpen;
    std::unique_ptr<sf::Sound> ambientMenuSound;
    std::unique_ptr<sf::Sound> ambientGameSound;
    float           ambientMenuCurrentVol = 0.f;
    float           ambientGameCurrentVol = 0.f;
    float           masterVolume = 100.f;
    float           uiVolume = 100.f;
    float           sfxVolume = 100.f;
    float           ambientVolume = 100.f;
    bool            audioMuted = false;
    float           masterVolumeBeforeMute = 100.f;

    // ── Stats ─────────────────────────────────────────────────────────────────
    void loadStats();
    void saveStats();

    // ═════════════════════════ STATE ═════════════════════════════════════════

    // Engine
    sf::RenderWindow window;
    sf::View         gameView;
    bool             fontLoaded = false;
    Phase            phase      = Phase::LOBBY;
    sf::Clock        frameClock;
    sf::Clock        uiClock;
    sf::Clock        phaseAnimClock;
    bool             uiDebugMode = false;

    // Per-frame input
    sf::Vector2f mousePos;
    sf::Vector2f clickPos;
    bool         mouseClicked = false;

    // Player palette (one colour per seat)
    static const sf::Color kColors[10];

    // ── Players ───────────────────────────────────────────────────────────────
    std::vector<Player> players;
    int humanIdx = 0;

    // ── AI Personalities ──────────────────────────────────────────────────────
    std::vector<AiPersonality> aiPersonalities;

    // ── Lobby ─────────────────────────────────────────────────────────────────
    int         lobbyCount      = 6;
    bool        lobbyDet        = true;
    bool        lobbyDoc        = true;
    bool        lobbyJoker      = false;
    bool        lobbyGodfather  = false;
    bool        lobbySilencer   = false;
    std::string humanName;
    bool        nameFocused     = false;

    // ── Role Reveal ───────────────────────────────────────────────────────────
    sf::Clock roleRevealClock;
    float     cardY      = 800.f;
    bool      blinkState = false;
    sf::Clock blinkClock;

    // ── Night ─────────────────────────────────────────────────────────────────
    sf::Clock nightClock;
    float nightDurationSec = 30.f;
    int  nightHumanTarget    = -1;
    bool nightConfirmed      = false;
    bool detectPopupOpen     = false;
    int  detectRevealIdx     = -1;
    std::vector<std::pair<std::string, Player::Role>> clues;
    std::vector<std::string>                          aiInvestigated;
    int  silencerTarget      = -1;
    int  silencedPlayerIdx   = -1;

    // ── Day ───────────────────────────────────────────────────────────────────
    std::string dayMsg;
    int         lastKilledIdx = -1;
    sf::Clock   zzzClock;
    int         roundNumber   = 1;

    // ── Discussion ────────────────────────────────────────────────────────────
    struct ChatMsg { std::string sender, text; sf::Color color; };
    std::vector<ChatMsg> chat;
    std::string          chatInput;
    sf::Clock            discClock;
    sf::Clock            aiChatClock;
    float                nextAiDelay = 3.f;
    float                discussionDurationSec = 60.f;
    float                aiChatMinDelaySec = 3.f;
    float                aiChatMaxDelaySec = 8.f;
    int                  aiAccusationBias = 0;
    std::map<int,int>    accusations;
    std::vector<std::vector<float>> aiSuspicion;
    std::vector<float>             publicSuspicion;
    std::vector<float>             publicCredibility;
    std::vector<int>               aiLastVoteTarget;
    int                            lastDoctorSavedTarget = -1;

    AIClient             aiClient;
    std::mutex           chatMutex;
    bool                 aiIsThinking = false;

    // ── Voting ────────────────────────────────────────────────────────────────
    int              humanVoteTarget = -1;
    bool             humanVoted      = false;
    bool             showConfirm     = false;
    bool             votesDone       = false;
    std::vector<int> voteCounts;
    std::vector<int> tallyShown;
    bool             tallyAnim   = false;
    sf::Clock        tallyClock;
    bool             showElim    = false;
    sf::Clock        elimClock;
    bool             elimContinueReady = false;
    int              eliminatedIdx     = -1;
    std::map<int,int> roundVotes;   // who voted whom this round (voterIdx → targetIdx)

    // ── Voting animation (sequential reveal) ────────────────────────────────────────
    struct VoteReveal { int voter; int target; };
    std::vector<VoteReveal> voteReveals;
    int  voteRevealIdx    = 0;
    bool revealingVotes   = false;
    sf::Clock voteRevealClock;

    // ── Game Over ─────────────────────────────────────────────────────────────
    std::string winFaction;
    bool        jokerWon = false;

    // ── Transition system ─────────────────────────────────────────────────────
    bool        transitioning = false;
    Phase       transTarget   = Phase::LOBBY;
    std::string transArgs;
    sf::Clock   transClock;
    bool        transSwapped  = false;
    static constexpr float TRANS_DUR = 0.45f;

    // ── Screen shake ──────────────────────────────────────────────────────────
    float     shakeIntensity = 0.f;
    sf::Clock shakeClock;
    static constexpr float SHAKE_DUR = 0.35f;

    // ── Particles ─────────────────────────────────────────────────────────────
    struct Particle {
        sf::Vector2f pos, vel;
        sf::Color color;
        float life, maxLife;
    };
    std::vector<Particle> particles;

    // ── Evidence ──────────────────────────────────────────────────────────────
    struct Evidence { std::string text; int round; };
    std::vector<Evidence> evidence;
    bool journalOpen = false;

    // ── Vote history ──────────────────────────────────────────────────────────
    struct VoteRecord { int round; std::map<int,int> whoVotedWhom; int eliminated; };
    std::vector<VoteRecord> voteHistory;
    struct KillEntry { int round; std::string name; std::string cause; };
    std::vector<KillEntry> killLog;

    // ── Tooltip ───────────────────────────────────────────────────────────────
    int   hoveredCardIdx = -1;
    float hoverTime      = 0.f;

    // ── Last Will system ──────────────────────────────────────────────────────
    std::vector<std::string> lastWills;
    bool lastWillOpen = false;

    // ── Night Sky ───────────────────────────────────────────────────────────
    struct Star { float x, y, baseBright, speed, phase; };
    std::vector<Star> nightStars;
    struct NightCloud { float x, y, w, speed, alpha; };
    std::vector<NightCloud> nightClouds;
    bool nightSkyInit = false;

    // ── Graveyard ───────────────────────────────────────────────────────────
    bool graveyardOpen = false;

    // ── Quick menu ───────────────────────────────────────────────────────────
    bool quickMenuOpen = false;
        // ── Instructions ─────────────────────────────────────────────────────────
        float instructionsScroll = 0.f;

    // ── Stats ─────────────────────────────────────────────────────────────────
    struct PlayerStats {
        int gamesPlayed = 0, wins = 0, losses = 0;
        int mafiaWins = 0, townWins = 0, jokerWins = 0;
    };
    PlayerStats stats;
};

#endif
