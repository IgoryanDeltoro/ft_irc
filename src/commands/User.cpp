#include "../../includes/Server.hpp"

void Server::user(Client *c, const Command &command)
{
    if (!c->getPassStatus()) return;
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 3 || command.getText().empty()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }
    const std::string &userName = params[0];
    if (userName.empty()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }
    const std::string &realName = command.getText();
    if (realName.empty()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }
    if (c->getRegStatus()) {
        sendNumericReply(c, ERR_ALREADYREGISTRED, "", "");
        return;
    }
    c->setUserName(userName);
    c->setRealName(realName);
    if (!c->getNick().empty() && c->getPassStatus()) {
        c->setRegStatus(true);
        sendWelcome(c);
    }
}
