#include "../../includes/Server.hpp"

void Server::ping(Client *c, const Command &command)
{
    std::string arg;

    if (!command.getText().empty())
        arg = command.getText();
    else if (!command.getParams().empty())
        arg = command.getParams()[0];

    if (arg.empty())
        return;

    c->enqueue_reply("PONG :" + arg + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}
