#include "../../includes/Server.hpp"

void Server::quit(Client *c, const Command &cmd)
{
    const std::string quitMsg = cmd.getText().empty() ? c->getNick(): cmd.getText();
    c->setQuit(quitMsg);
}
