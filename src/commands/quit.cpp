#include "../../includes/Server.hpp"

void Server::quit(Client *c, const Command &cmd)
{

    const std::string quitMsg = cmd.hasTrailing() ? cmd.getText() : c->getNick();

    std::map<int, Client *>::iterator it = _clients.find(c->getFD());
    if (it == _clients.end()) return;

    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == c->getFD())
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

    print_message("[" + getTime() + "] ", c->buildPrefix() + " leave with QUIT", BLUE, GREEN);

    removeClientFromAllChannels(c, quitMsg);

    if (!c->getNick().empty())
        _nicks.erase(c->getNickLower());

    close(it->first);
    delete it->second;
    _clients.erase(it);
}
