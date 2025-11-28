#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _is_registred(false), _pass_ok(false) {}

Client::~Client() {}

int Client::getFD() const { return this->_fd; }

const std::string &Client::getNick() const { return this->_nick; }

const std::string &Client::getUserName() const { return this->_userName; }

const std::string &Client::getRealName() const { return this->_realName; }

std::string &Client::getRecvBuff() { return this->_recv_buff; }

const bool &Client::getRegStatus() const { return this->_is_registred; }

const bool &Client::getPassStatus() const { return this->_pass_ok; }

void Client::setFD(int fd) { _fd = fd; }

void Client::setPassStatus(bool status) { _pass_ok = status; }

void Client::setNick(const std::string &nick) { _nick = nick; }

void Client::setUserName(const std::string &userName) { _userName = userName; }

void Client::setRealName(const std::string &realName) { _realName = realName; }

void Client::setRegStatus(bool status) { _is_registred = status; }

bool Client::operator!=(const Client &c) const
{
    return _fd != c._fd;
}