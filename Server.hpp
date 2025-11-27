#ifndef SERVER_HPP
#define SERVER_HPP
#include <iostream>
#include <vector>
#include <map>
#include "Client.hpp"

#define BECKLOG 10
#define BUFFER 4096

class Client;

class Server {
    private:
        int                         _listen_fd;
        std::string                 _port;
        std::string                 _password;
        std::vector<struct pollfd>  _pfds;
        std::map<int, Client*>      _clients;

        Server();
        Server(const Server &other);
        Server &operator=(const Server &other);

        int create_and_bind();
        void eccept_new_fd();
        void read_message_from(Client *);
        void process_line(Client *, std::string);
        void clean_fd(int);

    public:
        Server(const std::string &port, const std::string &password);
        ~Server();

        void run();

};

#endif