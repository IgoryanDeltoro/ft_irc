#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <iostream>
#include <set>
#include <map>

class Client;

class Channel {
    private:
        // int             _fd;

        std::string _name;
        std::string _topic;
        std::string _password;
        int _userLimit;

        bool _i;
        bool _t;
        bool _k;
        //bool o: Give/take channel operator privilege
        bool _l;

        std::set<std::string> _operators;
        std::map<std::string, Client*>  _users;
        std::set<std::string> _invited;

    public:
        Channel(const std::string &name, Client *client) : _name(name), _topic(""), _password(""), _userLimit(-1), _i(false), _t(false), _k(false), _l(false)
        {
            addUser(client);
            addOperator(client->getNick());
        }
        ~Channel() {}

        bool isI() const { return _i; }
        bool isT() const { return _t; }
        bool isK() const { return _k; }
        bool isL() const { return _l; }

        bool isUser(const std::string &nick) const { return _users.count(nick) != 0; }
        bool isOperator(const std::string &nick) const { return _operators.count(nick) != 0; }

        void addOperator(const std::string &nick)
        {
            _operators.insert(nick);
        };

        void removeOperator(const std::string &nick)
        {
            _operators.erase(nick);
        };

        void addUser(Client *c)
        {
            _users[c->getNick()] = c;
        }

        void removeUser(Client *c)
        {
            _users.erase(c->getNick());
            _operators.erase(c->getNick()); // если был оператором
            _invited.erase(c->getNick());   // если был приглашён
        }

        bool isInvited(const std::string &nick) const
        {
            return _invited.count(nick) != 0;
        }

        void addInvite(const std::string &nick)
        {
            _invited.insert(nick);
        }

        std::set<std::string> &getInvited()
        {
            return _invited;
        }

        std::set<std::string> &getOperators()
        {
            return _operators;
        }

        std::map<std::string, Client *> &getUsers()
        {
            return _users;
        }

        const int &getUserLimit() const
        {
            return _userLimit;
            ;
        }

        const std::string &getPassword() const
        {
            return _password;
        }

        const std::string &getTopic() const
        {
            return _topic;
        }

        void setTopic(const std::string &topic)
        {
            _topic = topic;
        }
};

#endif