#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <deque>
#include <set>

class Client {
    private:
        int                     _fd;
        std::string             _host;
        int                     _last_activity;
        bool                    _is_registred;
        bool                    _pass_ok;
        std::string             _nick;
        std::string             _nickLower;
        std::string             _userName;
        std::string             _realName;
        std::string             _recv_buff;
        std::deque<std::string> _send_msg;
        std::deque<time_t>      _cmd_timestamps; // for flood control
        std::set<std::string>   _channels;

        Client();

    public:
        Client(int fd, const std::string &);
        bool operator!=(const Client &c) const;
        ~Client();

        // getters
        int                     getFD() const;
        int                     getLastActivity() const;
        const std::string       &getNick() const;
        const std::string       &getHost() const;
        const std::string       &getNickLower() const;
        const std::string       &getUserName() const;
        const std::string       &getRealName() const;
        int                     getChannelSize() const;
        const std::set<std::string> &getChannels() const;
        std::deque<time_t> &getCmdTimeStamps();

        std::string             &getRecvBuff();
        const bool              &getRegStatus() const;
        const bool              &getPassStatus() const;
        std::deque<std::string> &getMessage();

        // setters
        void setFD(int fd);
        void setRegStatus(bool);
        void setPassStatus(bool);
        void setNick(const std::string &nick);
        void setNickLower(const std::string &nick);
        void setUserName(const std::string &user);
        void setRealName(const std::string &user);
        void enqueue_reply(const std::string &msg);
        void setCmdTimeStamps(const int &);
        void setLastActivity(const int &);
        std::string buildPrefix() const;
        void addToChannel(const std::string &name);
        void removeChannel(const std::string &name);
};

#endif
