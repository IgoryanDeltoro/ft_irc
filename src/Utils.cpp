#include "../includes/Server.hpp"

bool Server::isNickExists(const std::string &nick)
{
    return _nicks.count(nick) != 0;
}

bool Server::isClientAuth(Client *client)
{

    if (!client->getPassStatus())
    {
        // sendNumericReply(client, ERR_NEEDPASS, "", ""); TODO
        return false;
    }
    if (!client->getRegStatus())
    {
        // sendNumericReply(client, ERR_NOTREGISTERED, "", ""); TODO
        return false;
    }
    return true;
}

std::string Server::getTime() {
    time_t timestamp;
    time(&timestamp);

    std::string t(ctime(&timestamp));
    t.erase(t.size() - 1);
    return t;
}

void Server::print_message(const std::string &s1, const std::string &s2, const char * c1, const char *c2) {
    if (s1.empty()) return;
    std::cout << (c1 ? c1 : "") << s1 << (c1 ? RESET : "");
    if (s2.empty()) std::cout << std::endl;
    std::cout << (c2 ? c2 : "") << s2 << (c2 ? RESET : "");
    if (!s2.empty()) std::cout << std::endl;
}

int Server::flood_protection(Client *c, time_t curr_time)
{
    if (!c) return -1;

    c->setCmdTimeStamps(curr_time);
    bool isEmpty = c->getCmdTimeStamps().empty();

    while (!isEmpty && c->getCmdTimeStamps().front() + flood_win < curr_time)
    {
        c->getCmdTimeStamps().pop_front();
    }

    if ((int)c->getCmdTimeStamps().size() > flood_max)
    {
        std::string s =  ":" + _serverName + " NOTICE :Flooding detected\r\n";
        send(c->getFD(), s.c_str(), s.length(), 0);
        close_client(c->getFD());
        return -1;
    }
    return 0;
}

int Server::overflow_protection(Client *c) 
{
    if (c->getRecvBuff().size() > 512)
    {
        std::string s = ":" + _serverName + " NOTICE :Input buffer too long\r\n";
        send(c->getFD(), s.c_str(), s.length(), 0);
        close_client(c->getFD());
        return -1;
    }
    return 0;
}

void Server::sendWelcome(Client *c)
{
    std::string nick = c->getNick();

    std::string welcome = GREEN;
    welcome.append("\n__        _______ _     ____ ___  __  __ _____   _ \n");
    welcome.append(GREEN);
    welcome.append("\\ \\      / / ____| |   / ___/ _ \\|  \\/  | ____| | |\n");
    welcome.append(GREEN);
    welcome.append(" \\ \\ /\\ / /|  _| | |  | |  | | | | |\\/| |  _|   | |\n");
    welcome.append(GREEN);
    welcome.append("  \\ V  V / | |___| |__| |__| |_| | |  | | |___  |_|\n");
    welcome.append(GREEN);
    welcome.append("   \\_/\\_/  |_____|_____\\____\\___/|_|  |_|_____| (_)");
    welcome.append(RESET);

    std::string motd = BLUE;
    motd.append("Welcome to our IRC server!\n");
    motd.append(BLUE);
    motd.append("Type ");
    motd.append(GREEN);
    motd.append("HELP");
    motd.append(BLUE);
    motd.append(" to see all available commands.");
    motd.append(RESET);

    c->enqueue_reply(":" + _serverName + " 001 " + nick + " :" + welcome + "\r\n");
    c->enqueue_reply(":" + _serverName + " 002 " + nick + " :Your host is " + _serverName + "\r\n");
    c->enqueue_reply(":" + _serverName + " 003 " + nick + " :This server was created today\r\n");
    c->enqueue_reply(":" + _serverName + " 375 " + nick + " :- Message of the Day -\r\n");
    c->enqueue_reply(":" + _serverName + " 372 " + nick + " :- " + motd + "\r\n");
    c->enqueue_reply(":" + _serverName + " 376 " + nick + " :Have a wonderful chat session!\r\n");

    set_event_for_sending_msg(c->getFD(), true);
}

void Server::print_debug_message(Client *c, const Command &cmnd)
{
    if (_debug) 
    {
        print_message("Incoming MSG", " FROM", GREEN, YELLOW);
        std::string from = c->getNick().empty() ? " * " : c->getNick();
        print_message("[" + from + "]" RED " ==>> ", "{", YELLOW, YELLOW);
        print_message("  [ CMD  ]----| ", cmnd.getCommandStr(), MAGENTA, CYAN);
        std::vector<std::string>::const_iterator p = cmnd.getParams().begin();
        for (; p != cmnd.getParams().end(); ++p)
            print_message("  [TARGET]---------| ", *p, MAGENTA, CYAN);
        print_message("  [ TEXT ]----------------| ", cmnd.getText(), MAGENTA, CYAN);
        print_message("}", "", YELLOW, NULL);
    }
}