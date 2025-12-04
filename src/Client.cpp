#include "../includes/Client.hpp"

Client::Client(int fd) : _fd(fd), _last_activity(time(NULL)), _is_registred(false), _pass_ok(false) {}

Client::~Client() {}

int Client::getFD() const { return this->_fd; }

int Client::getLastActivity() const { return _last_activity; };

const std::string &Client::getNick() const { return this->_nick; }

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

void Client::setCmdTimeStamps(const time_t &t) { _cmd_timestamps.push_back(t); }

void Client::setNick(const std::string &nick) { _nick = nick; }

void Client::setUserName(const std::string &userName) { _userName = userName; }

void Client::setRealName(const std::string &realName) { _realName = realName; }

void Client::setRegStatus(bool status) { _is_registred = status; }

void Client::setLastActivity(const time_t &t) { _last_activity = t; };

bool Client::operator!=(const Client &c) const
{
    return _fd != c._fd;
}