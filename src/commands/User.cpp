#include "../../includes/Server.hpp"

void Server::user(Client *c, const Command &command)
{
    if (!c->getPassStatus()) {
        std::cout << MAGENTA << c->buildPrefix() << RED " password not set!\n" RESET;
        return;
    }
    
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 3 || command.getText().empty()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }

    const std::string &userName = params[0];
    const std::string &realName = command.getText();

    if (userName.empty()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "USER", "");
        return;
    }
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
    std::cout << MAGENTA << c->buildPrefix() << RESET " set username: " << userName << ", realname: " << realName << std::endl;

    if (!c->getNick().empty() && c->getPassStatus()) {
        c->setRegStatus(true);
        sendWelcome(c);
        std::cout << MAGENTA << c->buildPrefix() << GREEN " registered and welcome sent!" RESET << std::endl;
    }
}
