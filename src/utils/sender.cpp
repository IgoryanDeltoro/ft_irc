#include "../../includes/Server.hpp"

void Server::send_msg_to(Client *c)
{ 
    if (!c) return;

    while (!c->getMessage().empty())
    {
        std::string &s = c->getMessage().front();
        ssize_t n = send(c->getFD(), s.c_str(), s.length(), 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                close_client(c->getFD(), "Connection error");
                return;
            }
        }
        if (static_cast<size_t>(n) < s.size())
        {
            s.erase(0, n);
            break;
        }
        c->getMessage().pop_front();
    }
    set_event_for_sending_msg(c->getFD(), !c->getMessage().empty());
}