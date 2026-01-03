#include "../../includes/Server.hpp"

bool Server::isNickExists(const std::string &nick)
{
    return _nicks.count(nick) != 0;
}

bool Server::isClientAuth(Client *client)
{
    if (!client->getPassStatus()) return false;
    if (!client->getRegStatus()) return false;
    return true;
}

std::string Server::getTime() {
    time_t timestamp;
    time(&timestamp);

    std::string t(ctime(&timestamp));
    t.erase(t.size() - 1);
    return t;
}

void Server::check_timeouts()
{
    time_t nt = time(NULL);
    std::vector<int> to_close;
    std::map<int, Client *>::iterator it = _clients.begin();
    for (; it != _clients.end(); ++it)
    {
        if (nt - it->second->getLastActivity() > client_idle_timeout)
            to_close.push_back(it->first);
    }
    for (size_t i = 0; i < to_close.size(); i++)
        this->close_client(to_close[i], "Timeout");
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
        std::string s = ":" + _serverName + " NOTICE :Flood\r\n";
        send(c->getFD(), s.c_str(), s.length(), 0);
        close_client(c->getFD(), "Flood");
        return -1;
    }
    return 0;
}

int Server::overflow_protection(Client *c) 
{
    if (c->getRecvBuff().size() > 512)
    {
        std::string s = ":" + _serverName + " NOTICE :Input overflow\r\n";
        send(c->getFD(), s.c_str(), s.length(), 0);
        close_client(c->getFD(), "Input overflow");
        return -1;
    }
    return 0;
}

void Server::sendWelcome(Client *c)
{
    std::string nick = c->getNick();

    char buf[64];
    std::tm* tm = std::localtime(&getCreationDate());
    std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", tm);

    c->enqueue_reply(":" + _serverName + " 001 " + nick + " :Welcome to the Internet Relay Network " + c->buildPrefix() + "\r\n");
    c->enqueue_reply(":" + _serverName + " 002 " + nick + " :Your host is " + _serverName + ", running version 1.0\r\n");
    c->enqueue_reply(":" + _serverName + " 003 " + nick + " :This server was created " + std::string(buf) + "\r\n");
    c->enqueue_reply(":" + _serverName + " 004 " + nick + " " + _serverName + " 1.0 a itkol\r\n");
    c->enqueue_reply(":" + _serverName + " 375 " + nick + " :- " + _serverName + " Message of the day -\r\n");
    c->enqueue_reply(":" + _serverName + " 372 " + nick + " :- Enjoy your conversation!\r\n");
    c->enqueue_reply(":" + _serverName + " 372 " + nick + " :- Type HELP to see all available commands.\r\n");
    c->enqueue_reply(":" + _serverName + " 376 " + nick + " :End of /MOTD command\r\n");

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

void Server::sanitize_msg(std::string &msg)
{
    msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());
    msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
}

Client *Server::getClientByNick(const std::string &nick)
{
    std::map<int, Client *>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); it++)
    {
        if (it->second && it->second->getNickLower() == nick)
            return it->second;
    }
    return NULL;
}

void Server::set_event_for_sending_msg(int fd, bool doSend)
{
    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds[i].events = POLLIN;
            if (doSend)
                _pfds[i].events |= POLLOUT;
            break;
        }
    }
}

void Server::set_event_for_group_members(Channel *ch, bool doSend)
{
    std::map<std::string, Client *>::iterator member = ch->getUsers().begin();
    for (; member != ch->getUsers().end(); ++member)
    {
        for (size_t i = 0; i < _pfds.size(); i++)
        {
            if (_pfds[i].fd == member->second->getFD())
            {
                _pfds[i].events = POLLIN;
                if (doSend)
                    _pfds[i].events |= POLLOUT;
                break;
            }
        }
    }
}

Channel *Server::getChannel(const std::string &name) {
    std::map<std::string, Channel*>::iterator it;
    for (it = _channels.begin(); it != _channels.end(); it++)
    {
        if (it->second && it->second->getNameLower() == name)
            return it->second;
    }
    return NULL;
}
