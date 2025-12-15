#include "../../includes/Server.hpp"

void Server::help(Client *c)
{
    std::string msg;

    msg += YELLOW "Available commands:\n" RESET;
    msg += BLUE "----------------------------------------\n" RESET;
    msg += GREEN "PASS" RESET " <password>\n";
    msg += GREEN "NICK" RESET " <nickname>\n";
    msg += GREEN "USER" RESET " <username> 0 * :<realname>\n";
    msg += GREEN "JOIN" RESET " <#channel> [key]\n";
    msg += GREEN "MODE" RESET " <#channel> {[+|-] flags} [params]\n";
    msg += GREEN "TOPIC" RESET " <#channel> [:topic]\n";
    msg += GREEN "INVITE" RESET " <nickname> <#channel>\n";
    msg += GREEN "KICK" RESET " <#channel> <user> [:comment]\n";
    msg += GREEN "PRIVMSG" RESET " <target> :<text>\n";
    msg += BLUE "----------------------------------------\n" RESET;

    std::string nick = c->getNick().empty() ? "*" : c->getNick();
    c->enqueue_reply(":server NOTICE " + nick + " :" + msg + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}
