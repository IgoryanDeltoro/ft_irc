#include "../../includes/Server.hpp"

void Server::kick(Client *c, const Command &command)
{
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 2) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "KICK", "");
        return;
    }
    const std::vector<std::string> channelNames = _parser.splitByComma(params[0]);
    const std::vector<std::string> namesToKick = _parser.splitByComma(params[1]);

    if (channelNames.size() != 1 && channelNames.size() != namesToKick.size()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "KICK", "");
        return;
    }

    if (channelNames.size() == 1) {
        if (!_parser.isValidChannelName(channelNames[0])) {
            sendNumericReply(c, ERR_BADCHANMASK, "", channelNames[0]);
            return;
        }
        const std::string channelNameLower = _parser.ircLowerStr(channelNames[0]);
        if (_channels.count(channelNameLower) == 0) {
            sendNumericReply(c, ERR_NOSUCHCHANNEL, "", channelNames[0]);
            return;
        }
        Channel *ch = _channels[channelNameLower];
        if (!ch->isUser(c->getNickLower())) {
            sendNumericReply(c, ERR_NOTONCHANNEL, "", ch->getName());
            return;
        }
        if (!ch->isOperator(c->getNickLower())) {
            sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", ch->getName());
            return;
        }
        for (size_t i = 0; i < namesToKick.size(); i++) {
            const std::string nameToKickLower = _parser.ircLowerStr(namesToKick[i]);

            Client *userToKick = ch->findUserWithHistory(nameToKickLower);
            if (!userToKick) {
                sendNumericReply(c, ERR_USERNOTINCHANNEL, namesToKick[i], ch->getName());
                continue;
            }

            const std::string kickMsg = command.getText().empty() ? c->getNick() : command.getText();
            kickFromChannel(c, ch, userToKick, kickMsg);
        }
    }
    else {
        for (size_t i = 0; i < channelNames.size(); i++) {
            if (!_parser.isValidChannelName(channelNames[i])) {
                sendNumericReply(c, ERR_BADCHANMASK, "", channelNames[i]);
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
            Client *userToKick = ch->findUserWithHistory(nameToKickLower);
            if (!userToKick) {
                sendNumericReply(c, ERR_USERNOTINCHANNEL, namesToKick[i], ch->getName());
                continue;
            }
            
            const std::string kickMsg = command.getText().empty() ? c->getNick() : command.getText();
            kickFromChannel(c, ch, userToKick, kickMsg);
        }
    }
}

void Server::kickFromChannel(Client* c, Channel* ch, Client* target, const std::string& comment) {
    const std::string outMessage = ":" + c->buildPrefix() + " KICK " + ch->getName() + " " + target->getNick() + " :" + comment + "\r\n";

    ch->broadcast(NULL, outMessage);
    set_event_for_group_members(ch, true);

    target->removeChannel(ch->getNameLower());
    ch->removeInvite(target->getNickLower());
    ch->removeOperator(target->getNickLower());
    ch->removeUser(target->getNickLower());

    if (ch->getUsers().empty()) {
        _channels.erase(ch->getNameLower());
        delete ch;
    }
}