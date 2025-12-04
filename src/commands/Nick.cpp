#include "../../includes/Server.hpp"

void Server::nick(Client *c, const Command &command)
{
    if (!c->getPassStatus())
    {
        sendError(c, ERR_NEEDPASS, "");
        return;
    }

    std::vector<std::string> params = command.getParams();
    if (params.size() < 1)
    {
        sendError(c, ERR_NONICKNAMEGIVEN, "");
        return;
    }

    std::string newNick = params[0];
    if (!_parser.isValidNick(newNick))
    {
        sendError(c, ERR_ERRONEUSNICKNAME, newNick);
        return;
    }
    if (isNickExists(newNick, c))
    {
        sendError(c, ERR_NICKNAMEINUSE, newNick);
        return;
    }

    std::string oldNick = c->getNick();
    c->setNick(newNick);
    _nicks[newNick] = c;

    if (oldNick.empty())
    {
        std::cout << "User assigned nick: " << newNick << std::endl;

        if (!c->getNick().empty() && !c->getUserName().empty() && !c->getRealName().empty() && c->getPassStatus())
        {
            c->setRegStatus(true);
            sendWelcome(c);
        }
        return;
    }
    else
    {
        _nicks.erase(oldNick);

        std::string msg = ":" + oldNick + " NICK " + newNick + "\r\n";
        std::cout << msg;
        c->setRegStatus(true);
        // todo: to client + all in channels with client (now sendWelcome!)
        sendWelcome(c);
    }
}
