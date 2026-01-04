#include "../../includes/Server.hpp"

void Server::ping(Client *c)
{
    if (!c) return;

    c->setPongStatus(true);
    c->enqueue_reply("PING :" + _serverName + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}
