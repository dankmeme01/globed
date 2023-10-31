#include <Geode/Geode.hpp>
#include <data/player_account_data.hpp>

using namespace geode::prelude;

class ChatMessage : public CCNode {
public:
    bool init(const std::optional<PlayerAccountData>& accData, const std::string& message, float lineWidth);

    static ChatMessage* create(const PlayerAccountData& accData, const std::string& message, float lineWidth);
    static ChatMessage* createForServer(const std::string& message, float lineWidth, ccColor3B color);

    void setOpacity(GLubyte opacity);
    void setTextColor(ccColor3B color);

protected:
    CCLabelBMFont *nameLabel = nullptr, *line1 = nullptr, *line2 = nullptr, *line3 = nullptr;
    SimplePlayer* simplePlayer;
};