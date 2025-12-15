#include "Bot.hpp"
    
Bot::Bot(const std::string &ip, const std::string &port, const std::string &password) : _ip(ip),  _port(port), 
  _password(password), _nick("irc_bot"), _bot_name("irc_bot"), _connected_fd(-1), _ping_time(time(NULL)), _ping_wind(120) {

  _connected_fd = getsocketfd();
  if (_connected_fd == -1) throw std::runtime_error("Error: connection");

  if (connect(_connected_fd, (struct sockaddr *)&_serv_addr, sizeof(_serv_addr)) < 0) {
    close(_connected_fd);
    throw std::runtime_error( "Connection Failed");
  }

  if (fcntl(_connected_fd, F_SETFL, O_NONBLOCK) == -1)
    throw std::runtime_error("faild to make non-blocking mode");

  _pfd.fd = _connected_fd;
  _pfd.events = POLLIN;
  _pfd.revents = 0;
}

int Bot::getsocketfd() {
  int sock = -1;
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return -1;

  _serv_addr.sin_family = AF_INET;
  _serv_addr.sin_port = htons(atoi(_port.c_str()));

  if (inet_pton(AF_INET, _ip.c_str(), &_serv_addr.sin_addr) <= 0) return -1;

  return sock;
}

void Bot::run() {
 
  if (!_password.empty()) {
    _buffer = "PASS " + _password + "\r\n" + "NICK " + _nick + "\r\n" +"USER " + _password + " * 0 :" + _bot_name;
    _pfd.events |= POLLOUT;
  }

  
  while (1) {
    int ret = poll(&_pfd, 1, 1000);

    int curr_time = time(NULL);
    if (curr_time - _ping_time > _ping_wind) {
      _buffer = "PING tolsun.oulu.fi";
      _pfd.events |= POLLOUT;
      _ping_time = curr_time;
    }

    if (ret == 0) continue;
    if (ret < 0) throw std::runtime_error("poll error");

    if (_pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
      break;
    else if (_pfd.revents & POLLIN) {
      if (read_income_msg() == -1) break;
    }
      
    else if (_pfd.revents & POLLOUT)
      send_message_to_server();
  }
}

int Bot::read_income_msg() {
  char buf[512];

  while (true) {
    ssize_t bytes = recv(_connected_fd, buf, sizeof(buf), 0);
    if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        break;
    }
    if (bytes == 0)
      return -1;

    _buffer.append(buf, bytes);

    size_t pos;
    while ((pos = _buffer.find("\n")) != std::string::npos) {
      std::string line = _buffer.substr(0, pos);
      _buffer.erase(0, pos + 1);

      if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

      handleLine(line);
    }
  }
  return 0;
}

void Bot::send_message_to_server() {
  while (!_buffer.empty()) {
    std::string msg = _buffer + "\r\n";
    ssize_t n = send(_connected_fd, msg.c_str(), msg.size(), 0);
    _buffer.clear();
    _pfd.events = POLLIN;
  }
}

void Bot::handleLine(const std::string& line) {
    std::cout << "[IRC] " << line << std::endl;

    if (line.find("INVITE") != std::string::npos) {
      std::stringstream ss(line);
      while (ss >> _target) {
         if (_target[0] == '#' || _target[0] == '&')
          break;
      }
      if (_target.empty()) return ;

      _buffer = "PRIVMSG " + _target + " :Hello 👋 I am an irc bot";
      _pfd.events |= POLLOUT;
      return;
    }

    // PRIVMSG handling
    if (line.find("PRIVMSG") != std::string::npos) {
      std::stringstream ss(line);
      while (ss >> _target) {
          if (_target[0] == '#' || _target[0] == '&')
            break;
          else if (_target[0] == ':') {
            _target.erase(0, 1);
            break;
          }
      }

      std::string msg = line.substr(line.find(" :") + 2);

      // std::cout << "_target: " << _target << std::endl;

      if (msg == "hello" || msg == "hi") {
        _buffer = "PRIVMSG " + _target + " :Hi there 👋, I am an IRC bot";
      }
      else if (msg == "!time") {
        time_t t = time(NULL);
        _buffer = "PRIVMSG " + _target + " :" + std::string(ctime(&t));
      }
      _pfd.events |= POLLOUT;
    }
}

Bot::~Bot() {
  if (_connected_fd != -1)
    close(_connected_fd);
}