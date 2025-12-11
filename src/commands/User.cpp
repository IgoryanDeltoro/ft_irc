#include "../../includes/Server.hpp"

void Server::user(Client *c, const Command &command)
{
    if (!c->getPassStatus())
    {
        sendError(c, ERR_NEEDPASS, "", "");
        return;
    }
    std::vector<std::string> params = command.getParams();
    if (params.size() < 3 || command.getText().empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }

    std::string userName = params[0];
    _parser.trim(userName);
    if (userName.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }

    std::string realName = command.getText();
    _parser.trim(realName);
    if (realName.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }
    if (c->getRegStatus())
    {
        sendError(c, ERR_ALREADYREGISTRED, "", "");
        return;
    }

    c->setUserName(userName);
    c->setRealName(realName);

    if (!c->getNick().empty() && !c->getUserName().empty() && !c->getRealName().empty() && c->getPassStatus())
    {
        c->setRegStatus(true);
        sendWelcome(c);
    }
}
