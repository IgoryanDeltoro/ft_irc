#include "../../includes/Server.hpp"

void Server::invite(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    std::vector<std::string> params = command.getParams();
    if (params.size() < 2)
    {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "INVITE", "");
        return;
    }
    std::string nick = params[0];
    std::string channel = params[1];

    if (!_parser.isValidNick(nick))
    {
        sendNumericReply(c, ERR_NOSUCHNICK, nick, "");
        return;
    }
    if (!_parser.isValidChannelName(channel))
    {
        sendNumericReply(c, ERR_BADCHANMASK, "", channel);
        return;
    }

    std::string nickLower = _parser.ircLowerStr(nick);
    Client *invitee = getClientByNick(nickLower);
    if (!invitee)
    {
        sendNumericReply(c, ERR_NOSUCHNICK, nick, "");
        return;
    }

    std::string channelLower = _parser.ircLowerStr(channel);
    Channel *ch = NULL;
    if (_channels.count(channelLower) == 0)
    {
        // TODO check again (send only 341 to client?)
        std::string msg2 = ":server 341 " + c->getNick() + " " + invitee->getNick() + " " + ch->getName() + "\r\n";
        c->enqueue_reply(msg2);
        set_event_for_sending_msg(c->getFD(), true);
        return;
    }
    ch = _channels[channelLower];

    if (!ch->isUser(c->getNickLower()))
    {
        sendNumericReply(c, ERR_NOTONCHANNEL, "", channel);
        return;
    }

    if (ch->isI() && !ch->isOperator(c->getNickLower()))
    {
        sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", channel);
        return;
    }

    if (ch->isUser(nickLower))
    {
        sendNumericReply(c, ERR_USERONCHANNEL, invitee->getNick(), channel);
        return;
    }
    if (!ch->isInvited(nickLower))
    {
        ch->addInvite(nickLower);
    }

    std::string msg = c->buildPrefix() + " INVITE " + invitee->getNick() + " " + ch->getName() + "\r\n";

    // TODO check with msg = c->buildPrefix() + " INVITE " + invitee->getNick() + " :" + ch->getName() + "\r\n";
    invitee->enqueue_reply(msg);
    set_event_for_sending_msg(invitee->getFD(), true);

    std::string msg2 = ":server 341 " + c->getNick() + " " + invitee->getNick() + " " + ch->getName() + "\r\n";
    c->enqueue_reply(msg2);
    set_event_for_sending_msg(c->getFD(), true);

    // TODO:  RPL_AWAY
}
