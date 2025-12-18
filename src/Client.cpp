#include "../includes/Client.hpp"

Client::Client(int fd, const std::string &host) : _fd(fd), _host(host),
    _last_activity(time(NULL)), _is_registred(false), _pass_ok(false) {}

Client::~Client() {}

int Client::getFD() const { return this->_fd; }

int Client::getLastActivity() const { return _last_activity; };

const std::string &Client::getNick() const { return this->_nick; }

const std::string &Client::getNickLower() const { return this->_nickLower; }

const std::string &Client::getUserName() const { return this->_userName; }

const std::string &Client::getRealName() const { return this->_realName; }

std::string &Client::getRecvBuff() { return this->_recv_buff; }

const bool &Client::getRegStatus() const { return this->_is_registred; }

const bool &Client::getPassStatus() const { return this->_pass_ok; }
        
std::deque<time_t> &Client::getCmdTimeStamps() { return _cmd_timestamps; };

void Client::setFD(int fd) { _fd = fd; }

void Client::enqueue_reply(const std::string &msg) { _send_msg.push_back(msg); }

std::deque<std::string> &Client::getMessage() { return this->_send_msg; }

void Client::setPassStatus(bool status) { _pass_ok = status; }

void Client::setCmdTimeStamps(const int &t) { _cmd_timestamps.push_back(t); }

void Client::setNick(const std::string &nick) { _nick = nick; }

void Client::setNickLower(const std::string &nick) { _nickLower = nick; }

void Client::setUserName(const std::string &userName) { _userName = userName; }

void Client::setRealName(const std::string &realName) { _realName = realName; }

void Client::setRegStatus(bool status) { _is_registred = status; }

void Client::setLastActivity(const int &t) { _last_activity = t; };

bool Client::operator!=(const Client &c) const { return _fd != c._fd; }

const std::string &Client::getHost() const { return _host;}

std::string Client::buildPrefix() const { return ":" + _nick + "!" + _userName + "@" + _host; }

int Client::getChannelSize() const { return _channels.size(); }

const std::set<std::string> &Client::getChannels() const { return _channels; }

void Client::addToChannel(const std::string &name) { _channels.insert(name); }

void Client::removeChannel(const std::string &name) { _channels.erase(name); }
