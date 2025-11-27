#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <iostream>
#include <vector>
#include <map>

class Client;

class Channel {
    private:
        int             _fd;

        std::string     _topic;
        std::string     _password;
        int             _userLimit;

        bool            _inviteOnly;
        bool            _topicRestrictions;
        bool            _keyRestrictions;
        //bool o: Give/take channel operator privilege
        bool            _userLimitRestrictions;
    
        std::vector<Client*>  _operators;
        std::map<int, Client*>  _users;

    public:
        Channel(int fd, const Client* c);
        ~Channel();

        // getters
//         int         getFD() const;
//         std::string getNick() const;
//         std::string getUser() const;
//         std::string &getRecvBuff();
//         bool        getRegStatus() const;
//         bool        getPassStatus() const;

//         // setters
//         void setFD(int fd);
//         void setRegStatus();
//         void setPassStatus();
};

#endif