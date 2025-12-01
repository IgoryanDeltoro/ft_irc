#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <iostream>
#include <set>
#include <map>
#include "Client.hpp"

class Client;

class Channel {
    private:
        Channel();
        Channel(const Channel &copy);
        Channel &operator=(const Channel &src);

        std::string _name;
        std::string _topic;
        std::string _password;
        int _userLimit;

        bool _i;
        bool _t;
        bool _k;
        bool _l;

        std::set<std::string> _operators;
        std::set<std::string> _users;
        std::set<std::string> _invited;

    public:
        Channel(const std::string &name, Client *client);
        ~Channel();

        bool isI() const;
        bool isT() const;
        bool isK() const;
        bool isL() const;
        bool isUser(const std::string &nick) const;
        bool isOperator(const std::string &nick) const;
        bool isInvited(const std::string &nick) const;

        void addOperator(const std::string &nick);
        void addUser(const std::string &nick);
        void addInvite(const std::string &nick);

        void removeOperator(const std::string &nick);
        void removeUser(const std::string &nick);
        void removeInvite(const std::string &nick);

        std::set<std::string> &getInvited();
        std::set<std::string> &getOperators();
        std::set<std::string> &getUsers();
        const int &getUserLimit() const;

        const std::string &getPassword() const;
        const std::string &getTopic() const;

        void broadcast(Client *from, const std::string &msg);

        void setTopic(const std::string &topic);
        void setPassword(const std::string &password);
        void setK(const bool &k, const std::string &password);
        void setI(const bool &i);
        void setT(const bool &t);
        void setL(const int &limit);
};

#endif
