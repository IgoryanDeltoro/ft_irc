#include "../../includes/Server.hpp"

void Server::topic(Client *c, const Command &command)
{
    if (!isClientAuth(c)) return;
    const std::vector<std::string> params = command.getParams();
    if (params.size() < 1) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "TOPIC", "");
        return;
    }
    const std::string channelName = params[0];
    const std::string channelNameLower = _parser.ircLowerStr(channelName);
    if (_channels.count(channelNameLower) == 0) {
        sendNumericReply(c, ERR_NOSUCHCHANNEL, "", channelName);
        return;
    }
    Channel *ch = _channels[channelNameLower];
    if (!ch->isUser(c->getNickLower())) {
        sendNumericReply(c, ERR_NOTONCHANNEL, "", channelName);
        return;
    }
    if (command.getText().empty()) {
        if (!ch->hasTopic()) sendNumericReply(c, RPL_NOTOPIC, "", ch->getName());
        else sendNumericReply(c, RPL_TOPIC, ch->getTopic(), ch->getName());
        return;
    }
    if (ch->isT() && !ch->isOperator(c->getNickLower())) {
        sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", channelName);
        return;
    }
    const std::string newTopic = command.getText();
    ch->setTopic(newTopic, c->getNick());
    const std::string msg = c->buildPrefix() + " TOPIC " + ch->getName() + " :" + newTopic + "\r\n";
    ch->broadcast(NULL, msg);
    set_event_for_group_members(ch, true);
}
