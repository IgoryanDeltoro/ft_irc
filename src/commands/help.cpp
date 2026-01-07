#include "../../includes/Server.hpp"

void Server::help(Client *c)
{
    const std::string nick = c->getNick().empty() ? "*" : c->getNick();
    std::vector<std::string> lines;

    lines.push_back("            Available commands:");
    lines.push_back("-------------- Registration --------------");
    lines.push_back("PASS <password>");
    lines.push_back("NICK <nickname>");
    lines.push_back("USER <user> <mode> <unused> <realname>");
    lines.push_back("------------------------------------------");
    lines.push_back("JOIN <channel>{,<channel>} [<key>{,<key>}]");
    lines.push_back("TOPIC <channel> [<topic>]");
    lines.push_back("MODE <channel> (+|-)<modes> [params]");
    lines.push_back("INVITE <nickname> <channel>");
    lines.push_back("KICK <channel> <user> [<comment>]");
    lines.push_back("PRIVMSG <target> <text>");
    lines.push_back("PART <channel> [<message>]");
    lines.push_back("AWAY [<text>]");
    lines.push_back("QUIT [<message>]");
    lines.push_back("------------------------------------------");

    for (size_t i = 0; i < lines.size(); ++i)
    {
        c->enqueue_reply(
            ":" + _serverName + " NOTICE " + nick + " :" + lines[i] + "\r\n"
        );
    }

    set_event_for_sending_msg(c->getFD(), true);
}
