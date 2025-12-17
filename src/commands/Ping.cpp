#include "../../includes/Server.hpp"

void Server::ping(Client *c, const Command &command)
{
    const std::vector<std::string> params = command.getParams();
    if (params.size() < 1) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "PING", "");
        return;
    }
    if (params[0] != _serverName) return; // TODO: error?
    const std::string arg = command.getParams()[0];
    c->enqueue_reply("PONG " + _serverName + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}
