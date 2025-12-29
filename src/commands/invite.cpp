#include "../../includes/Server.hpp"

void Server::invite(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 2) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "INVITE", "");
        return;
    }
    const std::string &nick = params[0];
    const std::string nickLower = _parser.ircLowerStr(nick);
    const std::string &channel = params[1];
    const std::string channelLower = _parser.ircLowerStr(channel);

    Client *invitee = getClientByNick(nickLower);
    if (!invitee || !invitee->getRegStatus()) {
        sendNumericReply(c, ERR_NOSUCHNICK, nick, "");
        return;
    }

    if (_channels.count(channelLower)) {
        Channel *ch = _channels[channelLower];
        if (!ch->isUser(c->getNickLower())) {
            sendNumericReply(c, ERR_NOTONCHANNEL, "", ch->getName());
            return;
        }
        if (ch->isI() && !ch->isOperator(c->getNickLower())) {
            sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", ch->getName());
            return;
        }
        if (ch->isUser(nickLower)) {
            sendNumericReply(c, ERR_USERONCHANNEL, invitee->getNick(), ch->getName());
            return;
        }
        if (!ch->isInvited(nickLower)) ch->addInvite(nickLower);
    }

    const std::string msg = c->buildPrefix() + " INVITE " + invitee->getNick() + " " + channel + "\r\n";
    invitee->enqueue_reply(msg);
    set_event_for_sending_msg(invitee->getFD(), true);

    sendNumericReply(c, RPL_INVITING, nick, channel);

    if (invitee->isAway()) {
        sendNumericReply(c, RPL_AWAY, invitee->getNick(), invitee->getAwayMsg());
    }
}
