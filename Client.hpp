#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>

class Client {
    private:
        int             _fd;
        std::string     _nick;
        std::string     _user;
        std::string     _recv_buff;
        bool            _is_registred;
        bool            _pass_ok;
    
    public:
        Client(int fd);
        ~Client();

        // getters
        int         getFD() const;
        std::string getNick() const;
        std::string getUser() const;
        std::string &getRecvBuff();
        bool        getRegStatus() const;
        bool        getPassStatus() const;

        // setters
        void setFD(int fd);
        void setRegStatus();
        void setPassStatus();
};

#endif