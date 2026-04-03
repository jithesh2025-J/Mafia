#include "Game.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
void logCrash(const std::string& message)
{
    std::ofstream out("crash.log", std::ios::app);
    if (!out) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localTime = *std::localtime(&nowTime);
#endif

    out << "[" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] "
        << message << "\n";
}

enum class SimWinner {
    TOWN,
    MAFIA,
    JOKER
};

struct SimOptions {
    int matches = 100;
    int players = 6;
    int maxRounds = 40;
    bool detective = true;
    bool doctor = true;
    bool joker = false;
    bool godfather = false;
    bool silencer = false;
    bool useSeed = false;
    unsigned int seed = 0;
};

struct SimStats {
    int townWins = 0;
    int mafiaWins = 0;
    int jokerWins = 0;
    std::map<Player::Role, int> startingRoleCount;
    std::map<Player::Role, int> winningSurvivorCount;
};

bool isMafiaAligned(Player::Role role)
{
    return role == Player::Role::MAFIA ||
           role == Player::Role::GODFATHER ||
           role == Player::Role::SILENCER;
}

std::string roleName(Player::Role role)
{
    switch (role) {
        case Player::Role::MAFIA: return "Mafia";
        case Player::Role::GODFATHER: return "GodFather";
        case Player::Role::SILENCER: return "Silencer";
        case Player::Role::DOCTOR: return "Doctor";
        case Player::Role::DETECTIVE: return "Detective";
        case Player::Role::JOKER: return "Joker";
        case Player::Role::ROUINTAN: return "RouinTan";
        default: return "Villager";
    }
}

template <typename Rng>
int pickWeightedIndex(const std::vector<int>& candidates, const std::vector<float>& weights, Rng& rng)
{
    if (candidates.empty() || weights.empty() || candidates.size() != weights.size()) {
        return -1;
    }

    float sum = 0.f;
    for (float w : weights) {
        sum += std::max(0.f, w);
    }
    if (sum <= 0.f) {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(candidates.size()) - 1);
        return candidates[dist(rng)];
    }

    std::uniform_real_distribution<float> dist(0.f, sum);
    float roll = dist(rng);
    float acc = 0.f;
    for (size_t i = 0; i < candidates.size(); ++i) {
        acc += std::max(0.f, weights[i]);
        if (roll <= acc) {
            return candidates[i];
        }
    }
    return candidates.back();
}

std::vector<Player::Role> buildRoles(const SimOptions& opt, std::mt19937& rng)
{
    int mafiaNum = std::min(opt.players / 3, 2);
    if (mafiaNum < 1) mafiaNum = 1;

    std::vector<Player::Role> roles;
    if (opt.godfather && mafiaNum >= 1) {
        roles.push_back(Player::Role::GODFATHER);
        for (int i = 1; i < mafiaNum; ++i) roles.push_back(Player::Role::MAFIA);
    } else {
        for (int i = 0; i < mafiaNum; ++i) roles.push_back(Player::Role::MAFIA);
    }

    if (opt.silencer) roles.push_back(Player::Role::SILENCER);
    if (opt.detective) roles.push_back(Player::Role::DETECTIVE);
    if (opt.doctor) roles.push_back(Player::Role::DOCTOR);
    if (opt.joker) roles.push_back(Player::Role::JOKER);

    while (static_cast<int>(roles.size()) < opt.players) roles.push_back(Player::Role::VILLAGER);
    while (static_cast<int>(roles.size()) > opt.players) roles.pop_back();

    std::shuffle(roles.begin(), roles.end(), rng);
    return roles;
}

template <typename Rng>
SimWinner runOneSimulationMatch(const SimOptions& opt,
                                const std::vector<Player::Role>& roles,
                                SimStats& stats,
                                Rng& rng)
{
    const int n = static_cast<int>(roles.size());
    std::vector<bool> alive(n, true);
    std::unordered_set<int> knownMafia;

    for (Player::Role role : roles) {
        stats.startingRoleCount[role]++;
    }

    auto aliveIndices = [&](auto pred) {
        std::vector<int> out;
        for (int i = 0; i < n; ++i) {
            if (alive[i] && pred(i)) out.push_back(i);
        }
        return out;
    };

    auto checkWinner = [&]() -> std::optional<SimWinner> {
        int mafiaAlive = 0;
        int nonMafiaAlive = 0;
        for (int i = 0; i < n; ++i) {
            if (!alive[i]) continue;
            if (isMafiaAligned(roles[i])) mafiaAlive++;
            else nonMafiaAlive++;
        }
        if (mafiaAlive == 0) return SimWinner::TOWN;
        if (mafiaAlive >= nonMafiaAlive) return SimWinner::MAFIA;
        return std::nullopt;
    };

    for (int round = 0; round < opt.maxRounds; ++round) {
        if (auto winner = checkWinner(); winner.has_value()) {
            return *winner;
        }

        // Night phase: mafia kill, doctor save, detective info.
        auto mafiaAliveList = aliveIndices([&](int i) { return isMafiaAligned(roles[i]); });
        auto nonMafiaAliveList = aliveIndices([&](int i) { return !isMafiaAligned(roles[i]); });

        int mafiaTarget = -1;
        if (!mafiaAliveList.empty() && !nonMafiaAliveList.empty()) {
            std::vector<float> weights;
            for (int idx : nonMafiaAliveList) {
                float w = 1.5f;
                if (roles[idx] == Player::Role::DETECTIVE) w = 3.2f;
                else if (roles[idx] == Player::Role::DOCTOR) w = 2.8f;
                else if (roles[idx] == Player::Role::JOKER) w = 0.9f;
                weights.push_back(w);
            }
            mafiaTarget = pickWeightedIndex(nonMafiaAliveList, weights, rng);
        }

        int doctorSave = -1;
        auto doctorsAlive = aliveIndices([&](int i) { return roles[i] == Player::Role::DOCTOR; });
        if (!doctorsAlive.empty()) {
            auto aliveAll = aliveIndices([&](int) { return true; });
            std::vector<float> weights;
            for (int idx : aliveAll) {
                float w = isMafiaAligned(roles[idx]) ? 0.4f : 1.2f;
                if (roles[idx] == Player::Role::DETECTIVE) w += 1.8f;
                if (idx == doctorsAlive[0]) w += 0.8f;
                weights.push_back(w);
            }
            doctorSave = pickWeightedIndex(aliveAll, weights, rng);
        }

        if (mafiaTarget >= 0 && mafiaTarget != doctorSave) {
            alive[mafiaTarget] = false;
        }

        auto detectivesAlive = aliveIndices([&](int i) { return roles[i] == Player::Role::DETECTIVE; });
        if (!detectivesAlive.empty()) {
            int det = detectivesAlive[0];
            auto candidates = aliveIndices([&](int i) { return i != det; });
            if (!candidates.empty()) {
                std::uniform_int_distribution<int> d(0, static_cast<int>(candidates.size()) - 1);
                int investigated = candidates[d(rng)];
                if (isMafiaAligned(roles[investigated])) knownMafia.insert(investigated);
            }
        }

        if (auto winner = checkWinner(); winner.has_value()) {
            return *winner;
        }

        // Day vote phase.
        std::vector<int> votes(n, 0);
        auto aliveAll = aliveIndices([&](int) { return true; });
        std::uniform_real_distribution<float> chance(0.f, 1.f);

        for (int voter : aliveAll) {
            std::vector<int> candidates;
            std::vector<float> weights;
            for (int i = 0; i < n; ++i) {
                if (!alive[i] || i == voter) continue;
                candidates.push_back(i);

                float w = 1.f;
                if (isMafiaAligned(roles[voter])) {
                    if (isMafiaAligned(roles[i])) w = 0.f;
                    else {
                        w = 1.4f;
                        if (roles[i] == Player::Role::DETECTIVE) w += 2.1f;
                        if (roles[i] == Player::Role::DOCTOR) w += 1.6f;
                    }
                } else {
                    bool known = knownMafia.count(i) > 0;
                    if (known) w += 4.f;
                    else if (isMafiaAligned(roles[i])) w += 0.8f;
                    if (roles[i] == Player::Role::JOKER) w += 0.6f;

                    if (!knownMafia.empty() && chance(rng) < 0.58f) {
                        w = known ? 7.f : 0.25f;
                    }
                }
                weights.push_back(w);
            }

            int target = pickWeightedIndex(candidates, weights, rng);
            if (target >= 0) votes[target]++;
        }

        int best = -1;
        int bestVotes = 0;
        int tieCount = 0;
        for (int i = 0; i < n; ++i) {
            if (!alive[i]) continue;
            if (votes[i] > bestVotes) {
                bestVotes = votes[i];
                best = i;
                tieCount = 1;
            } else if (votes[i] == bestVotes && bestVotes > 0) {
                tieCount++;
            }
        }

        if (tieCount == 1 && best >= 0) {
            if (roles[best] == Player::Role::JOKER) {
                return SimWinner::JOKER;
            }
            alive[best] = false;
        }

        if (auto winner = checkWinner(); winner.has_value()) {
            return *winner;
        }
    }

    auto finalWinner = checkWinner();
    if (finalWinner.has_value()) {
        return *finalWinner;
    }
    return SimWinner::TOWN;
}

bool parseBoolFlag(const std::string& s, bool& out)
{
    if (s == "on" || s == "true" || s == "1") { out = true; return true; }
    if (s == "off" || s == "false" || s == "0") { out = false; return true; }
    return false;
}

bool parseSimulationArgs(int argc, char** argv, SimOptions& opt)
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--matches" && i + 1 < argc) {
            opt.matches = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--players" && i + 1 < argc) {
            opt.players = std::clamp(std::stoi(argv[++i]), 5, 10);
        } else if (arg == "--max-rounds" && i + 1 < argc) {
            opt.maxRounds = std::clamp(std::stoi(argv[++i]), 10, 200);
        } else if (arg == "--det" && i + 1 < argc) {
            if (!parseBoolFlag(argv[++i], opt.detective)) return false;
        } else if (arg == "--doc" && i + 1 < argc) {
            if (!parseBoolFlag(argv[++i], opt.doctor)) return false;
        } else if (arg == "--joker" && i + 1 < argc) {
            if (!parseBoolFlag(argv[++i], opt.joker)) return false;
        } else if (arg == "--godfather" && i + 1 < argc) {
            if (!parseBoolFlag(argv[++i], opt.godfather)) return false;
        } else if (arg == "--silencer" && i + 1 < argc) {
            if (!parseBoolFlag(argv[++i], opt.silencer)) return false;
        } else if (arg == "--seed" && i + 1 < argc) {
            opt.seed = static_cast<unsigned int>(std::stoul(argv[++i]));
            opt.useSeed = true;
        }
    }

    return true;
}

int runSimulationMode(int argc, char** argv)
{
    SimOptions opt;
    if (!parseSimulationArgs(argc, argv, opt)) {
        std::cerr << "Invalid simulation arguments. Use on/off for boolean flags.\n";
        return 2;
    }

    std::random_device rd;
    std::mt19937 rng(opt.useSeed ? opt.seed : rd());
    SimStats stats;

    for (int m = 0; m < opt.matches; ++m) {
        const auto roles = buildRoles(opt, rng);
        SimWinner winner = runOneSimulationMatch(opt, roles, stats, rng);

        if (winner == SimWinner::TOWN) stats.townWins++;
        else if (winner == SimWinner::MAFIA) stats.mafiaWins++;
        else stats.jokerWins++;
    }

    auto pct = [&](int v) {
        return (100.0 * static_cast<double>(v)) / static_cast<double>(opt.matches);
    };

    std::cout << "Simulation complete\n";
    std::cout << "Matches: " << opt.matches << "\n";
    std::cout << "Players: " << opt.players << "\n";
    std::cout << "Options: det=" << (opt.detective ? "on" : "off")
              << ", doc=" << (opt.doctor ? "on" : "off")
              << ", joker=" << (opt.joker ? "on" : "off")
              << ", godfather=" << (opt.godfather ? "on" : "off")
              << ", silencer=" << (opt.silencer ? "on" : "off") << "\n\n";

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Team win rates:\n";
    std::cout << "  Town : " << stats.townWins << " (" << pct(stats.townWins) << "%)\n";
    std::cout << "  Mafia: " << stats.mafiaWins << " (" << pct(stats.mafiaWins) << "%)\n";
    if (opt.joker) {
        std::cout << "  Joker: " << stats.jokerWins << " (" << pct(stats.jokerWins) << "%)\n";
    }

    std::cout << "\nAverage role count per match:\n";
    const std::array<Player::Role, 8> orderedRoles = {
        Player::Role::GODFATHER,
        Player::Role::MAFIA,
        Player::Role::SILENCER,
        Player::Role::DETECTIVE,
        Player::Role::DOCTOR,
        Player::Role::JOKER,
        Player::Role::VILLAGER,
        Player::Role::ROUINTAN
    };
    for (Player::Role r : orderedRoles) {
        auto it = stats.startingRoleCount.find(r);
        int count = (it == stats.startingRoleCount.end()) ? 0 : it->second;
        if (count == 0) continue;
        std::cout << "  " << roleName(r) << ": "
                  << (static_cast<double>(count) / static_cast<double>(opt.matches)) << "\n";
    }

    return 0;
}
}

int main(int argc, char** argv)
{
    try {
        bool simulate = false;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--simulate") {
                simulate = true;
                break;
            }
        }

        if (simulate) {
            return runSimulationMode(argc, argv);
        }

        Game game;
        game.run();
        return 0;
    } catch (const std::exception& ex) {
        logCrash(std::string("Unhandled std::exception: ") + ex.what());
        std::cerr << "Fatal error: " << ex.what() << "\n";
    } catch (...) {
        logCrash("Unhandled non-standard exception.");
        std::cerr << "Fatal error: unknown exception.\n";
    }

    return 1;
}

