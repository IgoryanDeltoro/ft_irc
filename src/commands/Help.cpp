#include "../../includes/Server.hpp"

void Server::help(Client *c)
{
    std::string pass = "command PASS <password>\n";
    std::string nick = "command NICK <nickname>\n";
    std::string user = "command USER <username> <hostname> <servername> :<realname>\n";
    std::string join = "command JOIN #channel key\n";
    std::string mode = "command MODE <#channel> {[+|-]|o|p|s|i|t|n|b|v} [<limit>] [<user>]\n";
    std::string topic = "command TOPIC <#channel>\n";
    std::string topic2 = "command TOPIC <#channel> :<topic>\n";
    std::string list = "command LIST <#channel> [<topic>]\n";
    std::string invite = "command INVITE <nickname> <#channel>\n";
    std::string kick = "command KICK <#channel> <user> [<comment>]\n";
    std::string privmsg = "command PRIVMSG <receiver>{,<receiver>} :<text to be sent>\n";

    if (!c->getRegStatus())
        c->enqueue_reply(YELLOW + pass + nick + user + RESET "\r\n");
    else
        c->enqueue_reply(YELLOW + join + mode + topic + list + invite + kick + privmsg + RESET "\r\n");

    set_event_for_sending_msg(c->getFD(), true);
    
    //     send_message_to_client(c->getFD(), ":" + pass + nick + user + "\r\n");
    // else
    //     send_message_to_client(c->getFD(), ":" + join + mode + topic + topic2 + list + invite + kick + privmsg + "\r\n");

}
