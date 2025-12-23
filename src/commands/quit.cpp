#include "../../includes/Server.hpp"

void Server::quit(Client *c, const Command &cmd)
{
    const std::string quitMsg = cmd.hasTrailing() ? cmd.getText() : c->getNick();
    c->setQuit(quitMsg);
}
