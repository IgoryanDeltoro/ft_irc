#include "../../includes/Server.hpp"

void Server::invite(Client *c, const Command &command)
{
    if (!isClientAuth(c)) return;
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 2) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "INVITE", "");
        return;
    }
    const std::string &nick = params[0];
    const std::string &channel = params[1];
    if (!_parser.isValidNick(nick)) {
        sendNumericReply(c, ERR_ERRONEUSNICKNAME, nick, "");
        return;
    }
    if (!_parser.isValidChannelName(channel)) {
        sendNumericReply(c, ERR_BADCHANMASK, "", channel);
        return;
    }
    const std::string nickLower = _parser.ircLowerStr(nick);
    Client *invitee = getClientByNick(nickLower);
    if (!invitee || !invitee->getRegStatus()) {
        sendNumericReply(c, ERR_NOSUCHNICK, nick, "");
        return;
    }
    const std::string channelLower = _parser.ircLowerStr(channel);
    if (_channels.count(channelLower)) {
        Channel *ch = _channels[channelLower];
        if (!ch->isUser(c->getNickLower())) {
            sendNumericReply(c, ERR_NOTONCHANNEL, "", channel);
            return;
        }
        if (ch->isI() && !ch->isOperator(c->getNickLower())) {
            sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", channel);
            return;
        }
        if (ch->isUser(nickLower)) {
            sendNumericReply(c, ERR_USERONCHANNEL, invitee->getNick(), channel);
            return;
        }
        if (!ch->isInvited(nickLower)) ch->addInvite(nickLower);
    }
    const std::string msg = c->buildPrefix() + " INVITE " + nick + " " + channel + "\r\n";
    invitee->enqueue_reply(msg);
    set_event_for_sending_msg(invitee->getFD(), true);
    sendNumericReply(c, RPL_INVITING, nick, channel);
}
