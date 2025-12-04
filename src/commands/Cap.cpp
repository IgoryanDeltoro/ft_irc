#include "../../includes/Server.hpp"

void Server::cap(Client *c, const Command &command)
{
    std::vector<std::string> params = command.getParams();
    if (params.size() == 0) return;
    if (params[0] == "LS") 
    {
        c->enqueue_reply(":server CAP * LS :\r\n");
        set_event_for_sending_msg(c->getFD());
        // send_message_to_client(c->getFD(), ":server CAP * END\r\n");
    }
    // else if (params[0] == "END") {}
}