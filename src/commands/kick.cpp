#include "../../includes/Server.hpp"

void Server::kick(Client *c, const Command &command)
{
    if (!isClientAuth(c)) return;
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 2) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "KICK", "");
        return;
    }
    const std::string &channelNamesRaw = params[0];
    const std::string &namesToKickRaw = params[1];
    const std::vector<std::string> channelNames = _parser.splitByComma(channelNamesRaw);
    const std::vector<std::string> namesToKick = _parser.splitByComma(namesToKickRaw);
    for (size_t i = 0; i < channelNames.size(); i++) {
        if (!_parser.isValidChannelName(channelNames[i])) {
            sendNumericReply(c, ERR_BADCHANMASK, "", channelNames[i]);
            continue;
        }
        if (i >= namesToKick.size()) {
            sendNumericReply(c, ERR_NEEDMOREPARAMS, "KICK", "");
            continue;
        }
        if (!_parser.isValidNick(namesToKick[i])) {
            sendNumericReply(c, ERR_ERRONEUSNICKNAME, namesToKick[i], "");
            continue;
        }
        const std::string channelNameLower = _parser.ircLowerStr(channelNames[i]);
        if (_channels.count(channelNameLower) == 0) {
            sendNumericReply(c, ERR_NOSUCHCHANNEL, "", channelNames[i]);
            continue;
        }
        Channel *ch = _channels[channelNameLower];
        if (!ch->isUser(c->getNickLower())) {
            sendNumericReply(c, ERR_NOTONCHANNEL, "", channelNames[i]);
            continue;
        }
        if (!ch->isOperator(c->getNickLower())) {
            sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", channelNames[i]);
            continue;
        }
        const std::string nameToKickLower = _parser.ircLowerStr(namesToKick[i]);
        Client *userToKick = ch->getUser(nameToKickLower);
        if (!userToKick || !userToKick->getRegStatus()) continue;
        const std::string outMessage = c->buildPrefix() + " KICK " + channelNames[i] + " " + userToKick->getNick() + " :" + command.getText() + "\r\n";
        ch->broadcast(NULL, outMessage);
        set_event_for_group_members(ch, true);
        userToKick->removeChannel(channelNameLower);
        ch->removeInvite(nameToKickLower);
        ch->removeOperator(nameToKickLower);
        ch->removeUser(nameToKickLower);
        if (ch->getUsers().empty()) {
            _channels.erase(channelNameLower);
            delete ch;
        }
    }
}
