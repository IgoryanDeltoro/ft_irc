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

void Server::print_message(const std::string &s1, const std::string &s2, const char * c1, const char *c2) {
    if (s1.empty()) return;
    bool isColor = (c1 || c2);
    std::cout << c1 << s1 << (isColor ? RESET : "") << (s2.empty() ? "\n" : "");
    std::cout << c2 << s2 << (isColor ? RESET : "") << std::endl;
}