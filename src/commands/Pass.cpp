#include "../../includes/Server.hpp"

void Server::pass(Client *c, const Command &command)
{
    std::vector<std::string> params = command.getParams();

    if (params.size() != 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "PASS");
        return;
    }
    if (c->getPassStatus())
    {
        sendError(c, ERR_PASSALREADY, "");
        return;
    }
    if (params[0] == _password)
        c->setPassStatus(true);
    else
    {
        sendError(c, ERR_INCORRECTPASSWORD, "");
        return;
    }

    // if (!c->getNick().empty() && !c->getUserName().empty() && !c->getRealName().empty())
    // {
    //     c->setRegStatus(true);
    //     c->enqueue_reply(GREEN "User is REGISTERED" RESET "\r\n");
    //     set_event_for_sending_msg(c->getFD());
    // }
}