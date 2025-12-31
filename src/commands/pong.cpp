#include "../../includes/Server.hpp"

void Server::pong(Client *c, const Command &command)
{
    if (command.getText().size() == 0 || command.getText() != _serverName) {
        sendNumericReply(c, ERR_NOORIGIN, "", "");
        return;
    }
    c->setPongStatus(false);
}