#include "../../includes/Server.hpp"

void Server::pass(Client *c, const Command &command)
{
    std::vector<std::string> params = command.getParams();
    if (params.size() < 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "PASS", "");
        return;
    }
    if (c->getRegStatus())
    {
        sendError(c, ERR_ALREADYREGISTRED, "", "");
        return;
    }
    if (c->getPassStatus())
    {
        sendError(c, ERR_PASSALREADY, "", "");
        return;
    }
    _parser.trim(params[0]);

    if (params[0] != _password)
    {
        sendError(c, ERR_PASSWDMISMATCH, "", "");
        return;
    }
    c->setPassStatus(true);
}
