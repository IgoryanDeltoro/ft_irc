#ifndef SERVER_HPP
# define SERVER_HPP
# include "Libraries.hpp"
# include "NumericReplies.hpp"
# include "MacroConstant.hpp"
# include "Parser.hpp"
# include "Client.hpp"
# include "Channel.hpp"
# include "Command.hpp"

class Parser;
class Client;
class Channel;
class Command;

class Server {
    private:
        bool                            _debug;
        time_t                          _listen_fd;
        time_t                          _last_timeout_check;
        Parser                          _parser;
        std::string                     _port;
        std::string                     _password;
        const std::string               _serverName;
        std::map<int, Client*>          _clients;
        std::map<std::string, Client*>  _nicks;
        std::map<std::string, Channel*> _channels; 
        std::vector<struct pollfd>      _pfds;

        static const int                timeout_interval = 5; 
        static const int                client_idle_timeout = 300; 
        static const int                flood_win = 10; 
        static const int                flood_max = 20; 
        
        Server();
        Server(const Server &other);
        Server &operator=(const Server &other);

        int         create_and_bind();
        int         flood_protection(Client *, time_t);
        int         overflow_protection(Client *);
        void        eccept_new_fd();
        void        read_message_from(Client *, int fd);
        void        send_msg_to(Client *, int fd);
        void        process_line(Client *, std::string &);
        void        close_client(int);
        void        set_event_for_group_members(Channel *ch, bool doSend);
        void        set_event_for_sending_msg(int fd, bool);
        void        check_timeouts();
        void        removeClientFromAllChannels(Client *c);

        Channel     *getChannel(const std::string &name);
        std::string getNumericReplyText(const NumericReply &r);


    public:
        Server(const std::string &port, const std::string &password);
        ~Server();
        
        void        run();
        void        help(Client *);
        void        pass(Client *, const Command &);
        void        nick(Client *, const Command &);
        void        user(Client *, const Command &);
        void        join(Client *, const Command &);
        void        mode(Client *, const Command &);
        void        kick(Client *, const Command &);
        void        topic(Client *, const Command &);
        void        invite(Client *, const Command &);
        void        cap(Client *, const Command &);
        void        privmsg(Client *, const Command &);
        void        ping(Client *, const Command &);
        void        print_debug_message(Client *, const Command &);
        bool        isClientAuth(Client *);
        bool        isNickExists(const std::string &);
        void        joinChannel(Client *, const std::string &, const std::string &);
        Client      *getClientByNick(const std::string &nick);
        std::string getTime();
        void        sendWelcome(Client *c);
        void        print_message(const std::string &s1, const std::string &s2, const char * c1, const char *c2);
        void        sendNumericReply(Client *c, NumericReply err, const std::string &arg, const std::string &channel);
        void        applyChannelMode(Client *c, Channel *channel, char f, bool adding, std::vector<std::string> &args, 
                        size_t &argIndex, std::string &addModeStr, std::string &removeModeStr, std::vector<std::string> &addModeArgs,
                        std::vector<std::string> &removeModeArgs, int &oLimit);
};

#endif