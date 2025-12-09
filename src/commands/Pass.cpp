#include "../../includes/Server.hpp"

void Server::pass(Client *c, const Command &command)
{
    std::vector<std::string> params = command.getParams();
    if (params.size() < 1) {
        std::cout << "params.size() < 1" << std::endl;
        sendError(c, ERR_NEEDMOREPARAMS, "PASS", "");
        return;
    }
    if (c->getRegStatus()) {
        std::cout << "c->getRegStatus()" << std::endl;

        sendError(c, ERR_ALREADYREGISTRED, "", "");
        return;
    }
    if (c->getPassStatus()) {
        std::cout << "c->getPassStatus()" << std::endl;

        sendError(c, ERR_PASSALREADY, "", "");
        return;
    }
    _parser.trim(params[0]);

    if (params[0] != _password) {
        std::cout << "params[0] " << params[0] << " != _password " << _password << std::endl;

        sendError(c, ERR_INCORRECTPASSWORD, "", "");
        return;
    }
    c->setPassStatus(true);
    std::cout << "PASS SET!!!!" << _password << std::endl;
}
