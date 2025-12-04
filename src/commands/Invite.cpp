#include "../../includes/Server.hpp"

void Server::invite(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
// Command:  INVITE
// Parameters: <nickname> <channel>

    std::vector<std::string>  params = command.getParams();
    // std::cout << "JOIN Command params:" << std::endl;
    // for (size_t i = 0; i < params.size(); i++)
    //     std::cout << i << ": " << params[i] << std::endl;

    if (params.size() < 2)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "INVITE");
        return;
    }
    std::string nick = params[0];
    std::string channel = params[1];

    if (!_parser.isValidNick(nick))
    {
        sendError(c, ERR_NOSUCHNICK, nick);
        return;
    }
    if (!_parser.isValidChannelName(channel))
    {
        sendError(c, ERR_BADCHANMASK, channel);
        return;
    }

    Channel *ch;
    if (_channels.count(channel) == 0)
    {
        sendError(c, ERR_NOSUCHCHANNEL, channel);
        return;
    }
    ch = _channels[channel];

    if (!ch->isUser(c->getNick()))
    {
        sendError(c, ERR_NOTONCHANNEL, channel);
        return;
    }

    if (ch->isI() && !ch->isOperator(c->getNick())) {
        sendError(c, ERR_CHANOPRIVSNEEDED, channel);
        return;
    }

    Client *invitee = getClientByNick(nick);
    if (!invitee) {
        sendError(c, ERR_NOSUCHNICK, nick);
        return;
    }
    if (ch->isUser(nick)) {
        sendError(c, ERR_USERONCHANNEL, channel);//todo error user!?
        return;
    }
    if (!ch->isInvited(nick)) {
        ch->addInvite(nick);
    }

    // :<inviter> INVITE <nickname> :<channel>
    std::stringstream msg;
    msg << ":" << c->getNick() << " INVITE " << invitee->getNick() << " :" << ch->getName();

    c->enqueue_reply(msg.str() + "\r\n");
    set_event_for_sending_msg(c->getFD());
}
