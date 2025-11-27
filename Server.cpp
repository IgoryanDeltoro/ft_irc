#include "Server.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <string>

Server::Server(const std::string &port, const std::string &password) : _listen_fd(-1), _port(port), _password(password) {
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

void Server::run() {
    std::cout << "Listening on port: " << _port << std::endl;

    while (1) {
        int ready = poll(&_pfds[0], _pfds.size(), 1000);
        if (ready < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll faild");
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
                    std::cout << "Clean filedescriptors\n";
                    clean_fd(p.fd);
                } else {
                    if (p.revents & POLLIN) read_message_from(c);
                    if (p.revents & POLLOUT) std::cout << "write outputs\n";
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
            if (errno == (EAGAIN | EWOULDBLOCK)) break;
            else break;
        }

        // make non-blocking mode
        if (fcntl(new_fd, F_SETFL, O_NONBLOCK) == -1) {
            close(new_fd);
            throw std::runtime_error("faild to make non-blocking mode");
        }

        _clients[new_fd] = new Client(new_fd);
    
        struct pollfd pa;
        pa.fd = new_fd;
        pa.events = POLLIN;
        pa.revents = 0;
        _pfds.push_back(pa);

        std::cout << "Accepted fd=" << new_fd << std::endl;
        send_message_to_client(new_fd, "Welcome to IRC chat.\r\n");
    }
}

void Server::read_message_from(Client *c) {
    if (!c) throw std::runtime_error("while reading incoming call");

    char buff[BUFFER] = {0};

    while (1) {
        ssize_t  bytes = recv(c->getFD(), buff, sizeof(buff), 0);
        if (bytes > 0) {
            c->getRecvBuff().append(buff, buff + bytes);

            // split lines on CRLF or LF
            size_t pos;
            while ((pos = c->getRecvBuff().find('\n')) != std::string::npos) {
                std::string line = c->getRecvBuff().substr(0, pos);
                c->getRecvBuff().erase(0, pos + 1); // erase buffer till LF ('\n')

                // strip CR if present
                // if last element CR then delete it
                if (!line.empty() && line[line.size() - 1] == '\r') { 
                    line.erase(line.size() - 1);
                }
                
                // handle process line
                process_line(c, line); 
            }
        } else if (bytes == 0) {
            // clean file descriptors
            clean_fd(c->getFD());
            break;
        } else {
            if (errno & (EAGAIN | EWOULDBLOCK)) break;
            // clean file descriptors
            clean_fd(c->getFD());
            break;
        }
    }
}

void Server::clean_fd(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return ;

    std::cout << "User " << it->second->getNick() << " leave session" << std::endl;
    
    // don't foget to remove from channels

    close(it->first);
    delete it->second;
    _clients.erase(it);

    // remove from _pfds
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

}

void Server::send_message_to_client(int fd, std::string msg) {
    if (fd < 0) return ;

    send(fd, msg.c_str(), msg.length(), 0);
}

void Server::process_line(Client *c, std::string line) {
    std::cout << "c->buff: " << c->getRecvBuff() << std::endl;
    std::cout << "line: " << line << std::endl;
}
