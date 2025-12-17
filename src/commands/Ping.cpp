#include "../../includes/Server.hpp"

void Server::ping(Client *c, const Command &command)
{
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 1) {
        sendNumericReply(c, ERR_NOORIGIN, "", "");
        return;
    }
    if (params[0] != _serverName) {
        sendNumericReply(c, ERR_NOSUCHSERVER, params[0], "");
        return; // TODO: error?
    }
    c->enqueue_reply("PONG " + _serverName + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}
