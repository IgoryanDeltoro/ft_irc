#ifndef BOT_HPP
#define BOT_HPP
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <poll.h>
#include <cerrno>
#include <fcntl.h>
#include <map>
#include <sstream>

class Bot {
    private:
        std::string         _ip;
        std::string         _port;
        std::string         _password;
        std::string         _nick;

        int                 _connected_fd;
        struct sockaddr_in  _serv_addr;
        std::string         _recv_buffer;
        std::string         _send_buffer;
        struct pollfd       _pfd;
        int                 _ping_time;
        int                 _ping_wind;

    
        Bot();
        Bot(const Bot &other);
        Bot &operator=(const Bot &other);

        int getsocketfd();
        // void sendCmd(const std::string& cmd);
        void handleLine(const std::string &);
        int read_income_msg();
        void send_message_to_server();
        std::map<std::string, std::string> parse_incomming_msg (const std::string &str);
        const std::string &get_param(std::map<std::string, std::string> &t, const std::string &str);

    public:
        Bot(const std::string &, const std::string &, const std::string &);
        ~Bot();

        void run();
};

#endif