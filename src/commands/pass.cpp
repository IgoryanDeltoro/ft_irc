#include "../../includes/Server.hpp"

void Server::pass(Client *c, const Command &command)
{
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 1) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "PASS", "");
        return;
    }
    if (c->getRegStatus()) {
        sendNumericReply(c, ERR_ALREADYREGISTRED, "", "");
        return;
    }
    if (params[0] != _password) {
        std::cout << MAGENTA << c->buildPrefix() << RED " incorrect password!\n" RESET;
        return;
    }
    c->setPassStatus(true);
    std::cout << MAGENTA << c->buildPrefix() << RESET " set password!\n";

}
