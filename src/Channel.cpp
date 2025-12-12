#include "../includes/Channel.hpp"

Channel::Channel(const std::string &name, const std::string &nameLower, Client *c) : _name(name), _nameLower(nameLower), _topic(""), _password(""), _userLimit(-1), _i(false), _t(false), _k(false), _l(false)
{
    addUser(c);
    addOperator(c->getNick());
}
Channel::~Channel() {}

bool Channel::isI() const { return _i; }
bool Channel::isT() const { return _t; }
bool Channel::isK() const { return _k; }
bool Channel::isL() const { return _l; }
bool Channel::isUser(const std::string &nick) const { return _users.count(nick) != 0; }
bool Channel::isOperator(const std::string &nick) const { return _operators.count(nick) != 0; }
bool Channel::isInvited(const std::string &nick) const { return _invited.count(nick) != 0; }

void Channel::addUser(Client *c) { _users[c->getNickLower()] = c; }
void Channel::addOperator(const std::string &nick) { _operators.insert(nick); }
void Channel::addInvite(const std::string &nick) { _invited.insert(nick); }

void Channel::removeOperator(const std::string &nick) { _operators.erase(nick); }
void Channel::removeInvite(const std::string &nick) { _invited.erase(nick); }
void Channel::removeUser(const std::string &nick) { _users.erase(nick); }

std::set<std::string> &Channel::getOperators() { return _operators; }
std::map<std::string, Client*> &Channel::getUsers() { return _users; }
std::set<std::string> &Channel::getInvited() { return _invited; }

const int &Channel::getUserLimit() const { return _userLimit; }
const std::string &Channel::getPassword() const { return _password; }
const std::string &Channel::getTopic() const { return _topic; }
const std::string &Channel::getName() const { return _name; }
const std::string &Channel::getNameLower() const { return _nameLower; }
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
    if (limit > 0) {
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
    std::map<std::string, Client*>::iterator it = _users.begin();
    for (; it != _users.end(); ++it) {
        if (from && from->getFD() == it->second->getFD()) continue;
        it->second->enqueue_reply(msg);
    }
}

Client *Channel::getUser(const std::string &nick)
{
    if (_users.count(nick))
        return _users[nick];
    return NULL;
}

std::string Channel::getAllModesString() const
{
    std::string mode = "+";
    if (_i) mode += "i";
    if (_t) mode += "t";
    if (_k) mode += "k";
    if (_l) mode += "l";

    std::string args;
    if (_k) args += " " + _password;
    if (_l) {
        std::stringstream ss;
        ss << _userLimit;
        args += " " + ss.str();
    }

    // for (std::map<std::string, Client *>::const_iterator it = _users.begin(); it != _users.end(); ++it)
    // {
    //     Client *clien = it->second;
    //     if (isOperator(clien->getNickLower()))
    //         args += " " + clien->getNick();
    // }

    return mode; //+ args;
}

bool Channel::hasTopic() const { return !_topic.empty(); }

const std::string &Channel::getTopicSetter() const { return _topicSetter; }

const time_t &Channel::getTopicTimestamp() const { return _topicTimestamp; }

void Channel::setTopic(const std::string &topic, const std::string &setter) {
    _topic = topic;
    _topicSetter = setter;
    _topicTimestamp = std::time(NULL);
}

const std::string Channel::getNamesList() const
{
    std::string list;
    for (std::map<std::string, Client *>::const_iterator it = _users.begin(); it != _users.end(); ++it) {
        Client *user = it->second;
        std::string prefix;
        if (isOperator(user->getNickLower())) prefix = "@";
        list += prefix + user->getNick() + " ";
    }
    return list;
}
