#include "../../includes/Server.hpp"

void Server::invite(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    std::vector<std::string> params = command.getParams();
    if (params.size() < 2)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "INVITE", "");
        return;
    }
    std::string nick = params[0];
    _parser.trim(nick);
    std::string channel = params[1];
    _parser.trim(channel);

    if (!_parser.isValidNick(nick))
    {
        sendError(c, ERR_NOSUCHNICK, nick, "");
        return;
    }
    if (!_parser.isValidChannelName(channel))
    {
        sendError(c, ERR_BADCHANMASK, "", channel);
        return;
    }

    std::string channelLower = _parser.ircLowerStr(channel);
    Channel *ch;
    if (_channels.count(channelLower) == 0)
    {
        sendError(c, ERR_NOSUCHCHANNEL, "", channel);
        return;
    }
    ch = _channels[channelLower];

    if (!ch->isUser(c->getNickLower()))
    {
        sendError(c, ERR_NOTONCHANNEL, "", channel);
        return;
    }

    if (ch->isI() && !ch->isOperator(c->getNickLower()))
    {
        sendError(c, ERR_CHANOPRIVSNEEDED, "", channel);
        return;
    }

    std::string nickLower = _parser.ircLowerStr(nick);
    Client *invitee = getClientByNick(nickLower);

    if (!invitee)
    {
        sendError(c, ERR_NOSUCHNICK, nick, "");
        return;
    }
    if (ch->isUser(nickLower))
    {
        sendError(c, ERR_USERONCHANNEL, invitee->getNick(), channel);
        return;
    }
    if (!ch->isInvited(nickLower))
    {
        ch->addInvite(nickLower);
    }

    std::string msg = c->buildPrefix() + " INVITE " + invitee->getNick() + " " + ch->getName() + "\r\n";
    invitee->enqueue_reply(msg);
    set_event_for_sending_msg(invitee->getFD(), true);

    std::string msg2 = ":server 341 " + c->getNick() + " " + invitee->getNick() + " " + ch->getName() + "\r\n";
    c->enqueue_reply(msg2);
    set_event_for_sending_msg(c->getFD(), true);

    // TODO:  RPL_AWAY
}
