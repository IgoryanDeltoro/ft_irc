#include "../../includes/Server.hpp"

void Server::nick(Client *c, const Command &command)
{
    if (!c->getPassStatus())
    {
        sendError(c, ERR_NEEDPASS, "", "");
        return;
    }

    std::vector<std::string> params = command.getParams();
    if (params.size() < 1)
    {
        sendError(c, ERR_NONICKNAMEGIVEN, "", "");
        return;
    }

    std::string newNick = params[0];
    _parser.trim(newNick);

    if (!_parser.isValidNick(newNick))
    {
        sendError(c, ERR_ERRONEUSNICKNAME, newNick, "");
        return;
    }

    const std::string oldNick = c->getNick();
    const std::string oldNickLower = c->getNickLower();
    const std::string newNickLower = _parser.ircLowerStr(newNick);

    if (oldNick == newNick)
        return;
    if (newNickLower != oldNickLower)
    {
        if (isNickExists(newNickLower))
        {
            sendError(c, ERR_NICKNAMEINUSE, newNick, "");
            return;
        }
    }

    if (!oldNick.empty())
        _nicks.erase(oldNickLower);

    c->setNick(newNick);
    c->setNickLower(newNickLower);
    _nicks[newNickLower] = c;

    if (oldNick.empty())
    {
        if (!c->getUserName().empty() && !c->getRealName().empty() && c->getPassStatus() && !c->getRegStatus())
        {
            c->setRegStatus(true);
            sendWelcome(c);
        }
        return;
    }

    std::string msg = ":" + oldNick + "!" + c->getUserName() + "@" + c->getHost() + " NICK " + newNick + "\r\n";

    c->enqueue_reply(msg);
    set_event_for_sending_msg(c->getFD(), true);

    if (!c->getRegStatus())
        return;

    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        Channel *ch = it->second;
        if (ch->isUser(oldNickLower))
        {
            ch->broadcast(c, msg);
            set_event_for_group_members(ch, true);

            ch->removeUser(oldNickLower);
            ch->addUser(c);

            if (ch->isOperator(oldNickLower))
            {
                ch->removeOperator(oldNickLower);
                ch->addOperator(newNickLower);
            }
            if (ch->isInvited(oldNickLower))
            {
                ch->removeInvite(oldNickLower);
                ch->addInvite(newNickLower);
            }
        }
    }
}
