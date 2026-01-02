#include "../includes/Client.hpp"

Client::Client(int fd, const std::string &host) : _fd(fd), _host(host), _last_activity(time(NULL)),
    _is_registred(false), _pass_ok(false), _away(false), _isQuit(false) {}
bool                    Client::operator!=(const Client &c) const { return _fd != c._fd; }
Client::~Client() {}

int                     Client::getFD() const { return this->_fd; }
int                     Client::getLastActivity() const { return _last_activity; };
const std::string       &Client::getNick() const { return this->_nick; }
const std::string       &Client::getNickLower() const { return this->_nickLower; }
// const std::string       &Client::getOldNickLower() const { return this->_oldNickLower; }
const std::string       &Client::getUserName() const { return this->_userName; }
const std::string       &Client::getRealName() const { return this->_realName; }
std::string             &Client::getRecvBuff() { return this->_recv_buff; }
const bool              &Client::getRegStatus() const { return this->_is_registred; }
const bool              &Client::getPassStatus() const { return this->_pass_ok; }    
std::deque<time_t>      &Client::getCmdTimeStamps() { return _cmd_timestamps; }
const std::string       &Client::getAwayMsg() const { return _awayMsg; }
const std::string       &Client::getHost() const { return _host;}
int                     Client::getChannelsSize() const { return _channels.size(); }
const                   std::set<std::string> &Client::getChannels() const { return _channels; }
std::deque<std::string> &Client::getMessage() { return this->_send_msg; }
const std::string       &Client::getQuitMsg() const { return _quitMsg; }

void                    Client::setFD(int fd) { _fd = fd; }
void                    Client::setPassStatus(bool status) { _pass_ok = status; }
void                    Client::setCmdTimeStamps(const int &t) { _cmd_timestamps.push_back(t); }
void                    Client::setNick(const std::string &nick) { _nick = nick; }
void                    Client::setNickLower(const std::string &nick) { _nickLower = nick; }
// void                    Client::setOldNickLower(const std::string &nick) { _oldNickLower = nick; }
void                    Client::setUserName(const std::string &userName) { _userName = userName; }
void                    Client::setRealName(const std::string &realName) { _realName = realName; }
void                    Client::setRegStatus(bool status) { _is_registred = status; }
void                    Client::setLastActivity(const int &t) { _last_activity = t; };

std::string             Client::buildPrefix() const { return _nick + "!" + _userName + "@" + _host; }

void                    Client::addToChannel(const std::string &name) { _channels.insert(name); }
void                    Client::removeChannel(const std::string &name) { _channels.erase(name); }
void                    Client::enqueue_reply(const std::string &msg) { _send_msg.push_back(msg); }

bool                    Client::isAway() const { return _away; }
bool                    Client::isQuit() const { return _isQuit; }

void Client::setAway(const std::string &msg) {
    _away = true;
    _awayMsg = msg;
}

void Client::unsetAway() {
    _away = false;
    _awayMsg.clear();
}

void Client::setQuit(const std::string &msg) {
    _isQuit = true;
    _quitMsg = msg;
}

void Client::addNickHistory(const std::string &oldNick) {
    NickHistory history;
    history.nick = oldNick;
    history.timestamp = time(NULL);
    _nickHistory.push_front(history);

    if (_nickHistory.size() > 10)
        _nickHistory.pop_back();
}

bool Client::hadNickRecently(const std::string &nick, time_t now) const {
    for (size_t i = 0; i < _nickHistory.size(); ++i) {
        if (_nickHistory[i].nick == nick && now - _nickHistory[i].timestamp < 10)
            return true;
    }
    return false;
}
