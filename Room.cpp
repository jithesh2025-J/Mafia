#include "Room.h"
#include <sstream>

namespace {

constexpr float kCardW = 180.f;
constexpr float kCardH = 100.f;
constexpr float kGap = 20.f;
constexpr float kOriginX = 100.f;
constexpr float kOriginY = 220.f;
constexpr unsigned kCols = 3;

std::vector<std::pair<int, sf::FloatRect>> buildAliveCardRegions(const std::vector<Player>& players)
{
    std::vector<std::pair<int, sf::FloatRect>> regions;
    unsigned slot = 0;
    for (size_t i = 0; i < players.size(); ++i) {
        if (!players[i].getIsAlive())
            continue;
        const unsigned col = slot % kCols;
        const unsigned row = slot / kCols;
        const float x = kOriginX + static_cast<float>(col) * (kCardW + kGap);
        const float y = kOriginY + static_cast<float>(row) * (kCardH + kGap);
        regions.push_back({static_cast<int>(i),
                           sf::FloatRect(sf::Vector2f{x, y}, sf::Vector2f{kCardW, kCardH})});
        ++slot;
    }
    return regions;
}

} // namespace

Room::Room(std::string name) : roomName(std::move(name)), currentPhase(GamePhases::START) {}

Room::~Room() = default;

void Room::readAction(Action, std::string) {}

void Room::readPlayers(std::string) {}

void Room::joinPlayers(std::string) {}

void Room::getState() {}

void Room::endVote() {}

std::string Room::getName() { return roomName; }

GamePhases Room::getCurrentPhase() { return currentPhase; }

GameState Room::getVisualState() const { return ::getVisualState(currentPhase); }

void Room::addMessage(const std::string& msg) { messages.push_back(msg); }

const std::vector<std::string>& Room::getMessages() const { return messages; }

void Room::clearMessages() { messages.clear(); }

void Room::checkPhase() {}

void Room::addJoker(int) {}

void Room::draw(sf::RenderTarget& target, const std::vector<Player>& players, const sf::Font& font,
                int selectedPlayerIndex, const std::vector<int>* voteCounts) const
{
    const auto regions = buildAliveCardRegions(players);
    for (const auto& [playerIndex, rect] : regions) {
        sf::RectangleShape card(sf::Vector2f(rect.size));
        card.setPosition(rect.position);
        card.setFillColor(sf::Color(45, 48, 62));
        if (playerIndex == selectedPlayerIndex) {
            card.setOutlineColor(sf::Color::Cyan);
            card.setOutlineThickness(4.f);
        } else {
            card.setOutlineColor(sf::Color(90, 90, 110));
            card.setOutlineThickness(2.f);
        }
        target.draw(card);

        const std::string& name = players[static_cast<size_t>(playerIndex)].getName();
        sf::Text nameText(font, name, 22);
        nameText.setFillColor(sf::Color::White);
        const auto nb = nameText.getLocalBounds();
        nameText.setPosition({rect.position.x + rect.size.x / 2.f - nb.size.x / 2.f - nb.position.x,
                              rect.position.y + 28.f});

        target.draw(nameText);

        if (voteCounts && static_cast<size_t>(playerIndex) < voteCounts->size()
            && (*voteCounts)[static_cast<size_t>(playerIndex)] > 0) {
            const std::string sub = "(" + std::to_string((*voteCounts)[static_cast<size_t>(playerIndex)])
                                    + " votes)";
            sf::Text subText(font, sub, 16);
            subText.setFillColor(sf::Color(180, 180, 200));
            const auto sb = subText.getLocalBounds();
            subText.setPosition({rect.position.x + rect.size.x / 2.f - sb.size.x / 2.f - sb.position.x,
                                 rect.position.y + 62.f});
            target.draw(subText);
        }
    }
}

std::optional<int> Room::handlePlayerCardClick(sf::Vector2f worldPosition,
                                                const std::vector<Player>& players) const
{
    const auto regions = buildAliveCardRegions(players);
    for (const auto& [idx, rect] : regions) {
        if (rect.contains(worldPosition))
            return idx;
    }
    return std::nullopt;
}
