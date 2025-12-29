#include "../../includes/Server.hpp"

void Server::read_message_from(Client *c)
{
    if (!c) return;

    char buff[BUFFER];
    while (1)
    {
        ssize_t bytes = recv(c->getFD(), buff, sizeof(buff), 0);
        if (bytes > 0)
        {

            c->getRecvBuff().append(buff, bytes);
            if (overflow_protection(c) == -1) return;

            time_t curr_time = time(NULL);
            c->setLastActivity(curr_time);

            size_t pos;
            while ((pos = c->getRecvBuff().find("\r\n")) != std::string::npos)
            {
                std::string line = c->getRecvBuff().substr(0, pos);
                c->getRecvBuff().erase(0, pos + 2); 
                
                if (flood_protection(c, curr_time) == -1) return;
                sanitize_msg(line);
                
                process_line(c, line);
                
                if (c->isQuit())
                {
                    close_client(c->getFD(), c->getQuitMsg());
                    return;
                }
            }
        }
        else if (bytes == 0)
        {
            close_client(c->getFD(), "Connection closed");
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            close_client(c->getFD(), "Connection error");
            break;
        }
    }
}