#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _is_registred(false), _pass_ok(false) {}

Client::~Client() {}

int Client::getFD() const { return this->_fd; }

std::string Client::getNick() const { return this->_nick; }

std::string Client::getUser() const { return this->_user; }

std::string &Client::getRecvBuff() { return this->_recv_buff; }

bool Client::getRegStatus() const { return this->_is_registred; }

bool Client::getPassStatus() const { return this->_pass_ok; }

void Client::setFD(int fd) { _fd = fd; }
