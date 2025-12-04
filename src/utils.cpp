#include "../includes/Server.hpp"

bool Server::isNickExists(const std::string &nick, Client *client)
{
    for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client *c = it->second;
        std::string lowerClientNick = _parser.ircLowerStr(nick);
        std::string lowerNickToFind = _parser.ircLowerStr(c->getNick());

        if (c != client && lowerNickToFind == lowerClientNick)
            return true;
    }
    return false;
}

bool Server::isClientAuth(Client *client)
{

    if (!client->getPassStatus())
    {
        sendError(client, ERR_NEEDPASS, "");
        return false;
    }
    if (!client->getRegStatus())
    {
        sendError(client, ERR_NOTREGISTERED, "");
        return false;
    }
    return true;
}
