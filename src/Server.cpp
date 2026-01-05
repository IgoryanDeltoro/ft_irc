#include "../includes/Server.hpp"

sig_atomic_t signaled = 1;

Server::Server(const std::string& port, const std::string& password) : _debug(DEBUG), _creationDate(time(NULL)), _listen_fd(-1),
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
                    close_client(p.fd, "Connection closed");
                }
                else if (p.revents & (POLLERR | POLLNVAL))
                {
                    close_client(p.fd, "Connection error");
                }
                else
                {
                    if (p.revents & POLLIN) read_message_from(c);
                    if (p.revents & POLLOUT) send_msg_to(c);
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
        if (new_fd < 0) break;

        if (fcntl(new_fd, F_SETFL, O_NONBLOCK) == -1)
        {
            close(new_fd);
            break;
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

void Server::process_line(Client *c, std::string &line)
{
    std::cout << YELLOW "resive from " GREEN << c->buildPrefix() << RESET " " << line << std::endl;

    if (line.empty() || line.size() > 510) return;
    Command cmnd = _parser.parse(line);

    print_debug_message(c, cmnd);
    
    if (cmnd.hasPrefix()) {
        if (!c->getRegStatus())
            return;
        std::string prefix = cmnd.getPrefix();
        size_t pos = prefix.find_first_of("!@");
        if (pos != std::string::npos)
            prefix = prefix.substr(0, pos);
        if (_parser.ircLowerStr(prefix) != c->getNickLower())
            return;
    }
    if (cmnd.getCommand() == NOT_VALID) { 
        return;
    }

    if (!c->getRegStatus()) {
        switch (cmnd.getCommand()) {
            case HELP: help(c); break;
            case PASS: pass(c, cmnd); break;
            case NICK: nick(c, cmnd); break;
            case USER: user(c, cmnd); break;
            case CAP: cap(c, cmnd); break;
            case PING: ping(c, cmnd); break;
            case QUIT: quit(c, cmnd); break;
            default: {
                sendNumericReply(c, ERR_NOTREGISTERED, "", "");
                break;
            }
        }
        return;
    }

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
        case PONG: pong(c, cmnd); break;
        case AWAY: away(c, cmnd); break;
        case PART: part(c, cmnd); break;
        case QUIT: quit(c, cmnd); break;
        default: {
            sendNumericReply(c, ERR_UNKNOWNCOMMAND, cmnd.getCommandStr(), "");
            break;
        };
    }
}

const time_t &Server::getCreationDate() const { return _creationDate; }

