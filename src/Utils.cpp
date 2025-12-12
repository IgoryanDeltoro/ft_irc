#include "../includes/Server.hpp"

bool Server::isNickExists(const std::string &nick)
{
    return _nicks.count(nick) != 0;
}

bool Server::isClientAuth(Client *client)
{

    if (!client->getPassStatus())
    {
        // sendNumericReply(client, ERR_NEEDPASS, "", ""); TODO
        return false;
    }
    if (!client->getRegStatus())
    {
        // sendNumericReply(client, ERR_NOTREGISTERED, "", ""); TODO
        return false;
    }
    return true;
}

std::string Server::getTime() {
    time_t timestamp;
    time(&timestamp);

    std::string t(ctime(&timestamp));
    t.erase(t.size() - 1);
    return t;
}
