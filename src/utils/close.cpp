#include "../../includes/Server.hpp"

void Server::close_client(int fd, const std::string &str)
{
    std::map<int, Client *>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return;

    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

    std::string prefix = it->second->buildPrefix();
    print_message("[" + getTime() + "] ", prefix + " leave", BLUE, GREEN);

    removeClientFromAllChannels(it->second, str);

    if (!it->second->getNick().empty()) 
        _nicks.erase(it->second->getNickLower());

    close(it->first);
    delete it->second;
    _clients.erase(it);
}

void Server::removeClientFromAllChannels(Client *c, const std::string &msg)
{
    const std::string nick = c->getNickLower();
    std::set<std::string> channels = c->getChannels();

    std::set<Client *> clients;
    for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it) {
        std::map<std::string, Channel *>::iterator chIt = _channels.find(*it);
        if (chIt == _channels.end()) continue;

        Channel *ch = chIt->second;
        ch->removeOperator(nick);
        ch->removeUser(nick);
        ch->removeInvite(nick);

        if (ch->getUsers().empty()) 
        {
            delete ch;
            _channels.erase(chIt);
        } 
        else 
        {
            std::map<std::string, Client*>::iterator it = ch->getUsers().begin();
            for (; it != ch->getUsers().end() ; ++it) {
                clients.insert(it->second);
            }
        }
    }

    for (std::set<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        Client *client = *it;
        client->enqueue_reply(":" + c->buildPrefix() + " QUIT :" + msg + "\r\n");
        set_event_for_sending_msg(client->getFD(), true);
    }
}