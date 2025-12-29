#include "../../includes/Server.hpp"

void Server::cap(Client *c, const Command &command)
{
    const std::vector<std::string> &params = command.getParams();
    if (params.size() == 0) return;
    if (params[0] == "LS") {
        c->enqueue_reply(":" + _serverName + " CAP * LS :\r\n");
        set_event_for_sending_msg(c->getFD(), true);
    }
}