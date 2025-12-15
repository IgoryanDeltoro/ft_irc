#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <iostream>
#include <set>
#include <map>
#include <sstream>
#include <vector>
#include "Client.hpp"

class Client;

class Channel {
    private:
        Channel();
        Channel(const Channel &copy);
        Channel &operator=(const Channel &src);

        std::string _name;
        std::string _nameLower;
        std::string _topic;
        std::string _password;
        int         _userLimit;
        std::string _topicSetter;
        int         _topicTimestamp;

        bool _i;
        bool _t;
        bool _k;
        bool _l;

        std::set<std::string> _operators; //nick lower
        std::map<std::string, Client*> _users; //nick lower
        std::set<std::string> _invited; //nick lower

    public:
        Channel(const std::string &name, const std::string &nameLower, Client *);
        ~Channel();

        bool isI() const;
        bool isT() const;
        bool isK() const;
        bool isL() const;
        bool isUser(const std::string &nick) const;
        bool isOperator(const std::string &nick) const;
        bool isInvited(const std::string &nick) const;

        void addOperator(const std::string &nick);
        void addUser(Client*);
        void addInvite(const std::string &nick);

        void removeOperator(const std::string &nick);
        void removeUser(const std::string &nick);
        void removeInvite(const std::string &nick);

        std::set<std::string> &getInvited();
        std::set<std::string> &getOperators();
        std::map<std::string, Client*> &getUsers();
        Client *getUser(const std::string &nick);
        const int &getUserLimit() const;

        const std::string &getPassword() const;
        const std::string &getTopic() const;
        const std::string &getName() const;
        const std::string &getNameLower() const;

        void broadcast(Client *, const std::string &);

        void setPassword(const std::string &password);
        void setK(const bool &k, const std::string &password);
        void setI(const bool &i);
        void setT(const bool &t);
        void setL(const int &limit);

        std::string getAllModesString() const;

        bool hasTopic() const;
        const std::string &getTopicSetter() const;
        const int &getTopicTimestamp() const;
        void setTopic(const std::string &topic, const std::string &setter);

        const std::string getNamesList() const;
};

#endif
