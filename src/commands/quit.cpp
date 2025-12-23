#include "../../includes/Server.hpp"

void Server::quit(Client *c, const Command &cmd)
{
    std::cout << "start QUIT" << std::endl;
    
    const std::string quitMsg = cmd.hasTrailing() ? cmd.getText() : c->getNick();

    std::map<int, Client *>::iterator it = _clients.find(c->getFD());
    if (it == _clients.end()) return;

    // for (size_t i = 0; i < _pfds.size(); ++i)
    // {
    //     if (_pfds[i].fd == c->getFD())
    //     {
    //         _pfds.erase(_pfds.begin() + i);
    //         break;
    //     }
    // }

    print_message("[" + getTime() + "] ", c->buildPrefix() + " leave with QUIT", BLUE, GREEN);

    c->isQuit = true;
    // removeClientFromAllChannels(c, quitMsg);

    // if (!c->getNick().empty())
    //     _nicks.erase(c->getNickLower());

    // close(it->first);
    // delete it->second;
    // it->second = NULL;
    // _clients.erase(it);

    // std::cout << RED "END OF QUIT" RESET << std::endl;
    
    
    // std::cout << "_pfds size: " << _pfds.size() << std::endl;
    // std::cout << "clients size: " << _clients.size() << std::endl;
    // std::cout << "nicks size: " << _nicks.size() << std::endl;
}
