#include "../includes/Server.hpp"

sig_atomic_t signaled = 1;

Server::Server(const std::string &port, const std::string &password) : _debug(DEBUG), _listen_fd(-1), 
    _last_timeout_check(time(NULL)), _port(port), _password(password), _serverName("irc.server")
{
    _listen_fd = create_and_bind();
    if (_listen_fd < 0)
        throw std::runtime_error("Failed to bind listening socket");
    if (listen(_listen_fd, BECKLOG) < 0)
    {
        close(_listen_fd);
        throw std::runtime_error("listen() failed");
    }

    if (fcntl(_listen_fd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("faild to make non-blocking mode");

    struct pollfd p;
    p.fd = _listen_fd;
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);
}

Server::~Server()
{
    std::map<int, Client *>::iterator it = _clients.begin();
    for (; it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    if (_listen_fd >= 0)
        close(_listen_fd);

    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        delete it->second;
    }
}

int Server::create_and_bind()
{
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, _port.c_str(), &hints, &res) != 0) return -1;

    int server_fd = -1;
    for (struct addrinfo *ad = res; ad != NULL; ad = ad->ai_next)
    {
        server_fd = socket(ad->ai_family, ad->ai_socktype, ad->ai_protocol);
        if (server_fd < 0)
            continue;

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(server_fd, ad->ai_addr, ad->ai_addrlen) == 0)
            break;
        close(server_fd);
        server_fd = -1;
    }
    freeaddrinfo(res);
    return server_fd;
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

void stop_listen(int param)
{
    if (param == 2)
        signaled = 0;
}

void Server::run()
{
    print_message("Listening on port: ", _port, GREEN, YELLOW);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGINT, stop_listen);

    while (signaled)
    {
        int ready = poll(&_pfds[0], _pfds.size(), 1000);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll faild");
        }

        time_t nt = time(NULL);
        if (nt - _last_timeout_check >= timeout_interval)
        {
            check_timeouts();
            _last_timeout_check = nt;
        }

        for (size_t j = 0; j < _pfds.size(); j++)
        {
            struct pollfd p = _pfds[j];
            if (p.revents == 0) continue;
            if (p.fd == _listen_fd && (p.revents & POLLIN))
            {
                eccept_new_fd(); 
            }
            else
            {
                std::map<int, Client *>::iterator it = _clients.find(p.fd);
                if (it == _clients.end()) continue;
                Client *c = it->second;


                if (p.revents & POLLHUP)
                {
                    std::cout << "POLLHUP" << std::endl;
                    close_client(p.fd, "Connection closed");
                }
                else if (p.revents & (POLLERR | POLLNVAL))
                {
                    std::cout << "POLLNVAL" << std::endl;
                    close_client(p.fd, "Connection error");
                }
                else
                {
                    std::cout << (p.revents & POLLIN ? "POLLIN" : "POLLOUT") << std::endl;
                    if (p.revents & POLLIN) read_message_from(c, p.fd);
                    if (p.revents & POLLOUT) send_msg_to(c, p.fd);
                }
            }
            p.revents = 0;
        }
    }
    print_message("\n", _serverName + " has been stoped.", RED, NULL);
}

void Server::eccept_new_fd()
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    while (1)
    {
        int new_fd = accept(_listen_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_fd < 0)
        {
            // TODO: always break !!!!!!!!!!!!!
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else break;
        }

        if (fcntl(new_fd, F_SETFL, O_NONBLOCK) == -1)
        {
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
        
        print_message("[" + getTime() + "] ", "host=" + std::string(ipStr), BLUE, GREEN);
    }
}

void Server::read_message_from(Client *c, int fd)
{
    if (!c || _clients.find(fd) == _clients.end()) return;

    char buff[BUFFER];
    while (1)
    {
        ssize_t bytes = recv(fd, buff, sizeof(buff), 0);
        if (bytes > 0)
        {

            c->getRecvBuff().append(buff, bytes);
            if (overflow_protection(c) == -1) return;

            time_t curr_time = time(NULL);
            c->setLastActivity(curr_time);

            size_t pos;
            while ((pos = c->getRecvBuff().find("\r\n")) != std::string::npos)
            {
                std::string line = c->getRecvBuff().substr(0, pos);
                c->getRecvBuff().erase(0, pos + 2); 
                
                if (flood_protection(c, curr_time) == -1) return;
                sanitize_msg(line);
                
                process_line(c, line);
                
                if (c->isQuit())
                {
                    close_client(fd, c->getQuitMsg());
                    return;
                }
            }
        }
        else if (bytes == 0)
        {
            close_client(fd, "Connection closed");
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            close_client(fd, "Connection error");
            break;
        }
    }
}

void Server::send_msg_to(Client *c, int fd)
{
    if (!c || _clients.find(fd) == _clients.end())
        return; // TODO: delete or not???

    while (!c->getMessage().empty())
    {
        std::string &s = c->getMessage().front();
        ssize_t n = send(c->getFD(), s.c_str(), s.length(), 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                close_client(c->getFD(), "Connection error");
                return;
            }
        }
        if (static_cast<size_t>(n) < s.size())
        {
            s.erase(0, n);
            break;
        }
        c->getMessage().pop_front();
    }
    set_event_for_sending_msg(c->getFD(), !c->getMessage().empty());
}

void Server::close_client(int fd, const std::string &str)
{
    std::cout << "start close_client" << std::endl;
   
    std::map<int, Client *>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return;

    for (size_t i = 0; i < _pfds.size(); ++i)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

    std::string prefix = it->second->buildPrefix();
    print_message("[" + getTime() + "] ", prefix + " leave", BLUE, GREEN);

    removeClientFromAllChannels(it->second, str);

    if (!it->second->getNick().empty()) 
        _nicks.erase(it->second->getNickLower());

    close(it->first);
    delete it->second;
    _clients.erase(it);

    std::cout << "finish close_client" << std::endl;
}

void Server::removeClientFromAllChannels(Client *c, const std::string &msg)
{
    std::cout << "start removeClientFromAllChannels" << std::endl;

    const std::string nick = c->getNickLower();
    std::set<std::string> channels = c->getChannels();

    std::set<Client *> clients;
    for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it) {
        std::map<std::string, Channel *>::iterator chIt = _channels.find(*it);
        if (chIt == _channels.end()) continue;

        Channel *ch = chIt->second;
        ch->removeOperator(nick);
        ch->removeUser(nick);
        ch->removeInvite(nick);

        if (ch->getUsers().empty()) 
        {
            delete ch;
            _channels.erase(chIt);
        } 
        else 
        {
            std::map<std::string, Client*>::iterator it = ch->getUsers().begin();
            for (; it != ch->getUsers().end() ; ++it) {
                clients.insert(it->second);
            }
        }
    }

    for (std::set<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        Client *client = *it;
        client->enqueue_reply(c->buildPrefix() + " QUIT :" + msg + "\r\n");
        set_event_for_sending_msg(client->getFD(), true);
    }
    
    std::cout << "end removeClientFromAllChannels" << std::endl;
}

void Server::process_line(Client *c, std::string &line)
{
    if (line.empty() || line.size() > 510) return;

    std::cout << YELLOW "resive " GREEN << c->buildPrefix() << RESET " : " RESET << line << std::endl;

    Command cmnd = _parser.parse(line);
    if (cmnd.getCommand() == NOT_VALID) return;
    if (cmnd.hasPrefix()) {
        if (!c->getRegStatus())
            return;
        std::string prefix = cmnd.getPrefix();
        size_t pos = prefix.find_first_of("!@");
        if (pos != std::string::npos)
            prefix = prefix.substr(0, pos);
        if (prefix != c->getNick())
            return;
    }
    if (cmnd.getCommand() == NOT_FOUND) { 
        sendNumericReply(c, ERR_UNKNOWNCOMMAND, cmnd.getCommandStr(), ""); 
        return;
    }

    print_debug_message(c, cmnd);

    switch (cmnd.getCommand()) {
        case HELP: help(c); break;
        case PASS: pass(c, cmnd); break;
        case NICK: nick(c, cmnd); break;
        case USER: user(c, cmnd); break;
        case JOIN: join(c, cmnd); break;
        case MODE: mode(c, cmnd); break;
        case KICK: kick(c, cmnd); break;
        case TOPIC: topic(c, cmnd); break;
        case INVITE: invite(c, cmnd); break;
        case CAP: cap(c, cmnd); break;
        case PRIVMSG: privmsg(c, cmnd); break;
        case PING: ping(c, cmnd); break;
        case AWAY: away(c, cmnd); break;
        case QUIT: quit(c, cmnd); break;
        default: break;
    }
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
