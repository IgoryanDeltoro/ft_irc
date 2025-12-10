#include "../includes/Server.hpp"
sig_atomic_t signaled = 1;

Server::Server(const std::string &port, const std::string &password) : _listen_fd(-1), 
    _last_timeout_check(time(NULL)), _port(port), _password(password) {
    _listen_fd = create_and_bind();
    if (_listen_fd < 0) throw std::runtime_error("Failed to bind listening socket");
    if (listen(_listen_fd, BECKLOG) < 0)
    {
        close(_listen_fd);
        throw std::runtime_error("listen() failed");
    }

    //make non-blocking
    // int flags = fcntl(_listen_fd, F_GETFL, O_NONBLOCK);
    // if (flags == -1) throw std::runtime_error("faild to make non-blocking mode");
    if (fcntl(_listen_fd, F_SETFL, O_NONBLOCK) == -1) throw std::runtime_error("faild to make non-blocking mode");

    struct pollfd p;
    p.fd = _listen_fd;
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);
}

Server::~Server() {
    std::map<int, Client*>::iterator it = _clients.begin();
    for (; it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    if (_listen_fd >= 0) close(_listen_fd);

    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        delete it->second;
    }
}

int Server::create_and_bind() {
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, _port.c_str(), &hints, &res) != 0) {
        std::cout << "Error: getaddrinfo" << std::endl;
        return -1;
    }

    int server_fd = -1;
    for (struct addrinfo *ad = res; ad != NULL; ad = ad->ai_next) {
        server_fd = socket(ad->ai_family, ad->ai_socktype, ad->ai_protocol);
        if (server_fd < 0) continue;

        // Attach socket to the port and reconnect
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Bind
        if (bind(server_fd, ad->ai_addr, ad->ai_addrlen) == 0) break;
        close(server_fd);
        server_fd = -1;
    }
    freeaddrinfo(res);
    return server_fd;
}

void Server::check_timeouts() {
    time_t nt = time(NULL);
    std::vector<int> to_close;
    std::map<int, Client*>::iterator it = _clients.begin();
    for (; it != _clients.end(); ++it) {
        if (nt - it->second->getLastActivity() > client_idle_timeout)
            to_close.push_back(it->first);
    }
    for (size_t i = 0; i < to_close.size(); i++) {
        this->close_client(to_close[i]);
    }
}

void stop_listen(int param) { 
    std::cout << RED "\nServer has been stoped." RESET << std::endl;
    if (param == 2) signaled = 0; 
}

void Server::run() {
    std::cout << BLUE "Listening on port: " GREEN << _port << RESET << std::endl;

    signal (SIGQUIT, SIG_IGN);
    signal (SIGINT, stop_listen);

    while (signaled) {
        int ready = poll(&_pfds[0], _pfds.size(), 1000);
        if (ready < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll faild");
        }

        // check timeouts periodically
        time_t nt = time(NULL);
        if (nt - _last_timeout_check >= timeout_interval) {
            check_timeouts();
            _last_timeout_check = nt;
        }

        /* Deal with array returned by poll(). */
        for (size_t j = 0; j < _pfds.size(); j++) {
            struct pollfd p = _pfds[j]; 
            if (p.revents == 0) continue;
            if (p.fd == _listen_fd && (p.revents & POLLIN)) {
                eccept_new_fd(); // rewrite below code in another function
            } else {
                std::map<int, Client*>::iterator it = _clients.find(p.fd); 
                if (it == _clients.end()) continue;
                Client *c = it->second;

                if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    close_client(p.fd);
                } else {
                    // std::cout << ((p.revents & POLLIN) ? "pollin" : "pollout") << std::endl;
                    if (p.revents & POLLIN) read_message_from(c, p.fd);
                    if (p.revents & POLLOUT) send_msg_to(c, p.fd);
                }
            }
            p.revents = 0;
        }
    }
}

void Server::eccept_new_fd() {
    // Accept a connection
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    while (1) {
        int new_fd = accept(_listen_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else break;
        }

        // make non-blocking mode
        if (fcntl(new_fd, F_SETFL, O_NONBLOCK) == -1) {
            close(new_fd);
            throw std::runtime_error("faild to make non-blocking mode");
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), ipStr, INET_ADDRSTRLEN);

        _clients[new_fd] = new Client(new_fd, std::string(ipStr));

        struct pollfd pa;
        pa.fd = new_fd;
        pa.events = POLLIN;
        pa.revents = 0;
        _pfds.push_back(pa);
        
        std::cout << BLUE "[" << getTime() << "] " GREEN "host=" << std::string(ipStr) << RESET << std::endl;
    }
}

void Server::read_message_from(Client *c, int fd) {
    if (!c || _clients.find(fd) == _clients.end()) return;

    char buff[BUFFER];

    while (1) {
        ssize_t bytes = recv(fd, buff, sizeof(buff), 0);

        if (bytes > 0) {
            c->getRecvBuff().append(buff, bytes);

            // Reject too long line (DoS protection)
            if (c->getRecvBuff().size() > 512) {
                std::string s = RED ":server NOTICE * :Input buffer too long" RESET "\r\n";
                send(fd, s.c_str(), s.length(), 0);
                close_client(fd);
                return;
            }

            time_t now = time(NULL);
            c->setLastActivity(now);

            // Parse full CRLF lines
            size_t pos;
            while ((pos = c->getRecvBuff().find("\r\n")) != std::string::npos) {

                std::string line = c->getRecvBuff().substr(0, pos);
                c->getRecvBuff().erase(0, pos + 2); // remove CRLF

                // flood protection per command
                c->setCmdTimeStamps(now);
                while (!c->getCmdTimeStamps().empty() &&
                       c->getCmdTimeStamps().front() + flood_win < now)
                    c->getCmdTimeStamps().pop_front();

                if ((int)c->getCmdTimeStamps().size() > flood_max) {
                    std::string s = RED ":server NOTICE * :Flooding detected" RESET "\r\n";
                    send(fd, s.c_str(), s.length(), 0);
                    close_client(fd);
                    return;
                }
                // Process the line
                process_line(c, line);
            }
        }
        else if (bytes == 0) {
            close_client(fd);
            break;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            close_client(fd);
            break;
        }
    }
}



void Server::send_msg_to(Client *c, int fd) {
    if (!c || _clients.find(fd) == _clients.end()) return ;

    while (!c->getMessage().empty()) {
        std::string &s = c->getMessage().front();
        ssize_t n = send(c->getFD(), s.c_str(), s.length(), 0);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                close_client(c->getFD());
                return;
            }
        }
        if (static_cast<size_t>(n) < s.size()) {
            s.erase(0, n);
            break;
        }
        c->getMessage().pop_front();
    }
    set_event_for_sending_msg(c->getFD(), !c->getRecvBuff().empty());
}

void Server::close_client(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return ;

    // remove from _pfds
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

    std::string def_nick = it->second->getNick().empty() ? "*_" : it->second->getNick();
    std::cout <<  BLUE "[" << getTime() << "] " GREEN << it->second->buildPrefix() << " leave" RESET << std::endl;

    removeClientFromAllChannels(it->second);

    if (!it->second->getNick().empty()) _nicks.erase(it->second->getNickLower());

    close(it->first);
    delete it->second;
    _clients.erase(it);
}

void Server::removeClientFromAllChannels(Client *c)
{
    std::string nick = c->getNickLower();

    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        Channel *ch = it->second;
        ch->removeOperator(nick);
        ch->removeUser(nick);
        ch->removeInvite(nick);
        // if (ch->getUsers().size() == 1) {

        // } else if (ch->getUsers().empty()) {
        //     delete ch;
        //     _channels.erase(it);
        // }
    }
}

void Server::sendError(Client *c, Error err, const std::string &arg, const std::string &channel)
{
    std::string message = getErrorText(err);
    std::string nick = c->getNick().empty() ? "*" : c->getNick();

    size_t pos;

    if ((pos = message.find("<command>")) != std::string::npos)
        message.replace(pos, 9, arg);
    else if ((pos = message.find("<nick>")) != std::string::npos)
        message.replace(pos, 6, arg);
    else if((pos = message.find("<char>")) != std::string::npos)
        message.replace(pos, 6, arg);
    else if((pos = message.find("<user>")) != std::string::npos)
        message.replace(pos, 6, arg);
    else if ((pos = message.find("<target>")) != std::string::npos)
        message.replace(pos, 8, arg);
    else if ((pos = message.find("<mask>")) != std::string::npos)
        message.replace(pos, 6, arg);

    if((pos = message.find("<channel>")) != std::string::npos)
        message.replace(pos, 9, channel);

    std::string s;
    std::stringstream out1;
    out1 << err;
    s = out1.str();

    c->enqueue_reply(RED ":server " + s + " " + nick + " " + message + RESET "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}

std::string Server::getErrorText(const Error &error)
{
    switch (error)
    {
    case ERR_KEYSET:
        return "<channel> :Channel key already set";
    case ERR_UNKNOWNMODE:
        return "<char> :is unknown mode char to me";
    case ERR_USERONCHANNEL:
        return "<user> <channel> :is already on channel";
    case ERR_USERNOTINCHANNEL:
        return "<nick> <channel> :They aren't on that channel";
    case ERR_CHANOPRIVSNEEDED:
        return "<channel> :You're not channel operator";
    case RPL_NOTOPIC:
        return "<channel> :No topic is set";
    case ERR_NOTONCHANNEL:
        return "<channel> :You're not on that channel";
    case ERR_NOTREGISTERED:
        return ": User has not registration";
    case ERR_BANNEDFROMCHAN:
        return "<channel> :Cannot join channel (+b)";
    case ERR_INVITEONLYCHAN:
        return "<channel> :Cannot join channel (+i)";
    case ERR_BADCHANNELKEY:
        return "<channel> :Cannot join channel (+k)";
    case ERR_CHANNELISFULL:
        return "<channel> :Cannot join channel (+l)";
    case ERR_BADCHANMASK:
        return "<channel> :Invalid channel name";
    case ERR_NOSUCHCHANNEL:
        return "<channel> :No such channel";
    case ERR_TOOMANYCHANNELS:
        return "<channel> :You have joined too many channels";
    case ERR_PASSALREADY:
        return ":Password already success";
    case ERR_NEEDPASS:
        return ":Server PASS required ";
    case ERR_PASSWDMISMATCH:
        return ":Password incorrect"; //"<client> :Password incorrect"
    case ERR_ALREADYREGISTRED:
        return ":You may not reregister";
    case ERR_NEEDMOREPARAMS:
        return "<command> :Not enough parameters";
    case ERR_NONICKNAMEGIVEN:
        return ":No nickname given";
    case ERR_ERRONEUSNICKNAME:
        return "<nick> :Erroneus nickname";
    case ERR_NICKNAMEINUSE:
        return "<nick> :Nickname is already in use";
    case ERR_NORECIPIENT:
        return "<nick> :No recipient given (PRIVMSG)";
    case ERR_NOTEXTTOSEND:
        return "<nick> :No text to send";
    case ERR_CANNOTSENDTOCHAN:
        return "<channel> :Cannot send to channel";
    case ERR_NOTOPLEVEL:
        return "<mask> :No toplevel domain specified"; // Если клиент отправляет PRIVMSG на некорректный канал/хост.
    case ERR_TOOMANYTARGETS:
        return "<target> :Duplicate recipients. No message delivered"; //<target> — это первый из дублирующихся или превышающих лимит получателей.
    case ERR_NOSUCHNICK:
        return "<nick> :No such nick/channel";
    default:
        return ":Error";
    }
}

void Server::process_line(Client *c, std::string line)
{
    std::cout << "line: " << line << std::endl;
    _parser.trim(line);
    if (line.empty())
    {
        return;
        // Command("", NOT_FOUND, "", ""); //???
    }

    Command cmnd = _parser.parse(line);

    if (cmnd.getCommand() == NOT_FOUND) {
        c->enqueue_reply("Command not found <" + line + ">\r\n");
        set_event_for_sending_msg(c->getFD(), true);
    }
    else if (cmnd.getCommand() == HELP) help(c);
    else if (cmnd.getCommand() == PASS) pass(c, cmnd);
    else if (cmnd.getCommand() == NICK) nick(c, cmnd);
    else if (cmnd.getCommand() == USER) user(c, cmnd);
    else if (cmnd.getCommand() == JOIN) join(c, cmnd);
    else if (cmnd.getCommand() == MODE) mode(c, cmnd);
    else if (cmnd.getCommand() == KICK) kick(c, cmnd);
    else if (cmnd.getCommand() == TOPIC) topic(c, cmnd);
    else if (cmnd.getCommand() == INVITE) invite(c, cmnd);
    else if (cmnd.getCommand() == CAP) cap(c, cmnd);
    else if (cmnd.getCommand() == PRIVMSG) privmsg(c, cmnd);
    else if (cmnd.getCommand() == PING) ping(c, cmnd);
}


void Server::sendWelcome(Client *c)
{
    std::string nick = c->getNick();

    c->enqueue_reply(":server 001 " + nick + " :Welcome to IRC server!\r\n");
    c->enqueue_reply(":server 002 " + nick + " :Your host is server\r\n");
    c->enqueue_reply(":server 003 " + nick + " :This server was created today\r\n");
    c->enqueue_reply(":server 375 " + nick + " :- server Message of the day -\r\n");
    c->enqueue_reply(":server 372 " + nick + " :- Welcome!\r\n");
    c->enqueue_reply(":server 376 " + nick + " :End of /MOTD command\r\n");
    set_event_for_sending_msg(c->getFD(), true);
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

void Server::set_event_for_sending_msg(int fd, bool doSend) {
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds[i].events = POLLIN;
            if (doSend) _pfds[i].events |= POLLOUT;
            break;
        } 
    }
}

void Server::set_event_for_group_members(Channel *ch, bool doSend) {
    std::map<std::string, Client*>::iterator member = ch->getUsers().begin();
    for (; member != ch->getUsers().end(); ++member) {
        for (size_t i = 0; i < _pfds.size(); i++) {
            if (_pfds[i].fd == member->second->getFD()) {
                _pfds[i].events = POLLIN;
                if (doSend) _pfds[i].events |= POLLOUT;
                break;
            }
        } 
    }
}
