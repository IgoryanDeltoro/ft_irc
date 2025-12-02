#include "Channel.hpp"

Channel::Channel(const std::string &name, Client *client) : _name(name), _topic(""), _password(""), _userLimit(-1), _i(false), _t(false), _k(false), _l(false)
{
    addUser(client->getNick());
    addOperator(client->getNick());
}
Channel::~Channel() {}

bool Channel::isI() const { return _i; }
bool Channel::isT() const { return _t; }
bool Channel::isK() const { return _k; }
bool Channel::isL() const { return _l; }
bool Channel::isUser(const std::string &nick) const { return _users.count(nick) != 0; }
bool Channel::isOperator(const std::string &nick) const { return _operators.count(nick) != 0; }
bool Channel::isInvited(const std::string &nick) const { return _invited.count(nick) != 0; }

void Channel::addUser(const std::string &nick) { _users.insert(nick); }
void Channel::addOperator(const std::string &nick) { _operators.insert(nick); }
void Channel::addInvite(const std::string &nick) { _invited.insert(nick); }


void Channel::removeOperator(const std::string &nick) { _operators.erase(nick); }
void Channel::removeInvite(const std::string &nick) { _invited.erase(nick); }
void Channel::removeUser(const std::string &nick)
{
    _users.erase(nick);
}

std::set<std::string> &Channel::getOperators() { return _operators; }
std::set<std::string> &Channel::getUsers() { return _users; }
std::set<std::string> &Channel::getInvited() { return _invited; }

const int &Channel::getUserLimit() const { return _userLimit; }
const std::string &Channel::getPassword() const { return _password; }
const std::string &Channel::getTopic() const { return _topic; }
const std::string &Channel::getName() const { return _name; }

void Channel::setTopic(const std::string &topic) { _topic = topic; }
void Channel::setPassword(const std::string &password) { _password = password; }

void Channel::setK(const bool &k, const std::string &password)
{
    if (k)
    {
        _k = true;
        _password = password;
    } else {
        _k = false;
        _password = "";
    }
}

void Channel::setI(const bool &i)
{
    if (i)
        _i = true;
    else
        _i = false;
}
void Channel::setT(const bool &t)
{
    if (t)
        _t = true;
    else
        _t = false;
}

void Channel::setL(const int &limit)
{
    if (limit) {
        _l = true;
        _userLimit = limit;
    }
    else {
        _l = false;
        _userLimit = -1;
    }
}

void Channel::broadcast(Client *from, const std::string &msg)
{
    (void)from;
    (void)msg;

    // std::set<std::string>::iterator it = _users.begin();

    // for (; it != _users.end(); ++it) {
    //     if (from && from->getFD() != it->second->getFD())
    //         it->second->enqueue_reply(msg);
    // }
}
