#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>
#include <deque>

class Client {
    private:
        int                     _fd;
        bool                    _is_registred;
        bool                    _pass_ok;
        std::string             _nick;
        std::string             _userName;
        std::string     _realName;
        std::string             _recv_buff;
        std::deque<std::string> _send_msg;
    
    public:
        Client(int fd);
        bool operator!=(const Client &c) const;
        ~Client();

        // getters
        int                     getFD() const;
        const std::string             &getNick() const;
        const std::string             &getUserName() const;
        const std::string &getRealName() const;

        std::string             &getRecvBuff();
        const bool             &getRegStatus() const;
        const bool             &getPassStatus() const;
        std::deque<std::string> &getMessage();

        // setters
        void setFD(int fd);
        void setRegStatus(bool);
        void setPassStatus(bool);
        void setNick(const std::string &nick);
        void setUserName(const std::string &user);
        void setRealName(const std::string &user);
        void enqueue_reply(const std::string &msg);
};

#endif