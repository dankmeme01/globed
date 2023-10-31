#include <Geode/Geode.hpp>
#include <data/player_account_data.hpp>

using namespace geode::prelude;

class ChatMessage : public CCNode {
public:
    bool init(const PlayerAccountData& accData, const std::string& message, float lineWidth);

    static ChatMessage* create(const PlayerAccountData& accData, const std::string& message, float lineWidth);

    void setOpacity(GLubyte opacity);

protected:
    CCLabelBMFont *nameLabel = nullptr, *line1 = nullptr, *line2 = nullptr, *line3 = nullptr;
    SimplePlayer* simplePlayer;
};