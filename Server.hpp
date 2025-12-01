#ifndef SERVER_HPP
#define SERVER_HPP
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>
#include <map>
#include "Parser.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Command.hpp"

# define RED "\e[31m"
# define GREEN "\e[32m"
# define YELLOW "\e[33m"
# define BLUE "\e[34m"
# define MAGENTA "\e[35m"
# define CYAN "\e[36m"
# define WHITE "\e[37m"
# define RESET "\e[0m"

#define BECKLOG 10
#define BUFFER 4096

class Parser;
class Client;
class Channel;
class Command;

enum Error
{
    // pass
    ERR_INCORRECTPASSWORD = 1001,
    ERR_NEEDPASS = 1002,
    ERR_PASSALREADY = 1003,
    ERR_NOTREGISTERED = 1004,

    RPL_NOTOPIC = 331,
    ERR_NOSUCHCHANNEL = 403,
    ERR_TOOMANYCHANNELS = 405,
    ERR_NONICKNAMEGIVEN = 431,
    ERR_ERRONEUSNICKNAME = 432,
    ERR_NICKNAMEINUSE = 433,
    ERR_USERNOTINCHANNEL = 441,
    ERR_NOTONCHANNEL = 442,
    ERR_USERONCHANNEL = 443,
    ERR_NEEDMOREPARAMS = 461,
    ERR_ALREADYREGISTRED = 462,
    ERR_CHANNELISFULL = 471,
    ERR_INVITEONLYCHAN = 473,
    ERR_BANNEDFROMCHAN = 474,
    ERR_BADCHANNELKEY = 475,
    ERR_BADCHANMASK = 476,
    ERR_CHANOPRIVSNEEDED = 482,

    // IRC numeric error replies
    ERR_NORECIPIENT = 411,
    ERR_NOTEXTTOSEND = 412,
    ERR_CANNOTSENDTOCHAN = 404,
    ERR_NOTOPLEVEL = 413,
    ERR_WILDTOPLEVEL = 414,
    ERR_TOOMANYTARGETS = 407,
    ERR_NOSUCHNICK = 401,
    ERR_KEYSET = 467,
    ERR_UNKNOWNMODE = 472,

    // IRC numeric reply
    RPL_AWAY = 301,
};

class Server {
    private:
        int                         _listen_fd;
        std::string                 _port;
        std::string                 _password;
        std::vector<struct pollfd>  _pfds;
        std::map<int, Client*>      _clients;
        std::map<std::string, Channel *> _channels;
        Parser _parser;
        std::map<std::string, Client*>  _nicks;

        Server();
        Server(const Server &other);
        Server &operator=(const Server &other);

        int create_and_bind();
        void eccept_new_fd();
        void read_message_from(Client *);
        void send_msg_to(Client *);
        void process_line(Client *, std::string);
        void clean_fd(int);
        std::string getErrorText(const Error &error);
        ssize_t send_message_to_client(int, std::string);
        void set_event_for_sending_msg(int fd);

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

        bool isNickExists(const std::string &, Client *);
        void joinChannel(Client *, const std::string &, const std::string &);
        bool isClientAuth(Client *);
        Client *getClientByNick(const std::string &nick);
        void kickClientFromChannel(Channel &, Client *);

        void sendError(Client *c, Error err, const std::string &arg);

        void sendWelcome(Client *c);

};

#endif