#include "../../includes/Server.hpp"

void Server::away(Client *c, const Command &cmd)
{
    if (!c->getRegStatus())
        return;

    if (!cmd.hasTrailing()) {
        c->unsetAway();
        sendNumericReply(c, RPL_UNAWAY, "", "");
        return;
    }
    c->setAway(cmd.getText());
    sendNumericReply(c, RPL_NOWAWAY, "", "");
}
