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
    
        std::set<int>  _operators;
        std::map<int, Client*>  _users;
        std::set<int> _invited;

    public:
        Channel(const std::string &name, Client *client) : _name(name), _topic(""), _password(""), _userLimit(-1), _i(false), _t(false), _k(false), _l(false)
        {
            addUser(client);
            addOperator(client->getFD());
        }
        ~Channel() {}

        bool isI() const { return _i; }
        bool isT() const { return _t; }
        bool isK() const { return _k; }
        bool isL() const { return _l; }

        bool isUser(Client *c) const { return _users.count(c->getFD()) != 0; }
        bool isOperator(int fd) const { return _operators.count(fd) != 0; }

        void addOperator(int fd)
        {
            _operators.insert(fd);
        };

        void removeOperator(int fd)
        {
            _operators.erase(fd);
        };

        void addUser(Client *c)
        {
            _users[c->getFD()] = c;
        }

        void removeUser(Client *c)
        {
            _users.erase(c->getFD());
            _operators.erase(c->getFD()); // если был оператором
            _invited.erase(c->getFD());   // если был приглашён
        }

        bool isInvited(int fd) const
        {
            return _invited.count(fd) != 0;
        }

        void addInvite(int fd)
        {
            _invited.insert(fd);
        }

        const std::set<int> &getInvited() const
        {
            return _invited;
        }

        const std::set<int> &getOperators() const
        {
            return _operators;
        }

        const std::map<int, Client *> &getUsers() const
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