#include "../includes/Server.hpp"

bool Server::isNickExists(const std::string &nick)
{
    return _nicks.count(nick) != 0;
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
