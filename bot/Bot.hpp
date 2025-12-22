#ifndef BOT_HPP
# define BOT_HPP
# include <arpa/inet.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <cstring>
# include <iostream>
# include <stdlib.h>
# include <poll.h>
# include <cerrno>
# include <fcntl.h>
# include <map>
# include <signal.h>

class Bot {
    private:
        std::string                         _ip;
        std::string                         _port;
        std::string                         _password;
        std::string                         _nick;
        std::string                         _serv_name;
        std::string                         _recv_buffer;
        std::string                         _send_buffer;
        struct sockaddr_in                  _serv_addr;
        struct pollfd                       _pfd;
        int                                 _connected_fd;
        int                                 _ping_time;

        static const int                    _ping_wind = 60;
        static const int                    _pong_recv_time = 2;

        void                                invite(const std::string &);
        void                                privmsg(const std::string &, const std::string &);
        void                                pong(const std::string &);
        void                                ping();
        int                                 getsocketfd();
        void                                handleLine(const std::string &);
        int                                 read_income_msg();
        void                                send_message_to_server();
        std::map<std::string, std::string>  parse_incoming_msg (const std::string &str);
        const std::string                   &get_param(std::map<std::string, std::string> &t, const std::string &str);

        Bot();
        Bot(const Bot &other);
        Bot &operator=(const Bot &other);

    public:
        Bot(const std::string &, const std::string &, const std::string &);
        ~Bot();

        void                                run();
};

#endif