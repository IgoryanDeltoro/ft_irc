#include "../../includes/Server.hpp"

void Server::ping(Client *c, const Command &command)
{
    std::vector<std::string> params = command.getParams();
    if (params.size() < 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "PING", "");
        return;
    }

    std::string arg = command.getParams()[0];
    _parser.trim(arg);

    c->enqueue_reply("PONG " + arg + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}
