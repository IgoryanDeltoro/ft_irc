#ifndef SERVER_HPP
#define SERVER_HPP
#include "Libraries.hpp"
#include "NumericReplies.hpp"
#include "MacroConstant.hpp"
#include "Parser.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Command.hpp"

class Parser;
class Client;
class Channel;
class Command;


class Server {
    private:
        int                         _listen_fd;
        int                         _last_timeout_check;
        std::string                 _port;
        std::string                 _password;
        std::vector<struct pollfd>  _pfds;
        std::map<int, Client*>      _clients;
        std::map<std::string, Client*>  _nicks;  //nick lower
        std::map<std::string, Channel*> _channels;  //ch name Lower lower
        Parser _parser;

        static const int timeout_interval = 5; // seconds
        static const int client_idle_timeout = 300; // seconds
        static const int flood_win = 10; // seconds
        static const int flood_max = 20; // max commands in window

        Server();
        Server(const Server &other);
        Server &operator=(const Server &other);

        int create_and_bind();
        void eccept_new_fd();
        void read_message_from(Client *, int fd);
        void send_msg_to(Client *, int fd);
        void process_line(Client *, std::string);
        void close_client(int);
        std::string getErrorText(const Error &error);
        ssize_t send_message_to_client(int, std::string);
        void set_event_for_sending_msg(int fd, bool);
        void check_timeouts();
        void removeClientFromAllChannels(Client *c);

    public:
        Server(const std::string &port, const std::string &password);
        ~Server();

        void run();

        void help(Client *);
        void pass(Client *, const Command &);
        void nick(Client *, const Command &);
        void user(Client *, const Command &);
        void join(Client *, const Command &);
        void mode(Client *, const Command &);
        void kick(Client *, const Command &);
        void topic(Client *, const Command &);
        void invite(Client *, const Command &);
        void cap(Client *, const Command &);
        void privmsg(Client *, const Command &);
        void ping(Client *, const Command &);

        bool isNickExists(const std::string &);
        void joinChannel(Client *, const std::string &, const std::string &);
        bool isClientAuth(Client *);
        Client *getClientByNick(const std::string &nick);

        void sendError(Client *c, Error err, const std::string &arg, const std::string &channel);
        void sendWelcome(Client *c);
        std::string getNamesList(Client *c, Channel *ch);
};

#endif