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
        std::string             _user;
        std::string             _recv_buff;
        std::deque<std::string> _send_msg;
    
    public:
        Client(int fd);
        ~Client();

        // getters
        int                     getFD() const;
        std::string             getNick() const;
        std::string             getUser() const;
        std::string             &getRecvBuff();
        bool                    getRegStatus() const;
        bool                    getPassStatus() const;
        std::deque<std::string> &getMessage();

        // setters
        void setFD(int fd);
        void setNick(std::string);
        void setRegStatus();
        void setPassStatus();
        void enqueue_reply(const std::string &msg);
};

#endif