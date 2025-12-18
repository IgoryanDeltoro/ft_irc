#include "Bot.hpp"

sig_atomic_t signaled = 1;

Bot::Bot(const std::string &ip, const std::string &port, const std::string &password) : _ip(ip),  _port(port), 
  _password(password), _nick("ircBot"), _connected_fd(-1), _ping_time(time(NULL)), _ping_wind(120) {

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

  void stop_listen(int param) {
    std::cout << "\nBot has been stoped." << std::endl;
    if (param == 2)
        signaled = 0;
  }

void Bot::run() {
 
  signal(SIGQUIT, SIG_IGN);
  signal(SIGINT, stop_listen);

  if (!_password.empty()) {
    std::string pass = "PASS " + _password + "\r\n"; 
    _send_buffer.append(pass + "NICK " + _nick + "\r\nUSER " + _nick + " * 0 :" + _nick + "\r\n");
    _pfd.events |= POLLOUT;
  }
  
  while (signaled) {
    int ready = poll(&_pfd, 1, 1000);

    int curr_time = time(NULL);
    if (curr_time - _ping_time > _ping_wind) {
      _send_buffer += "PING " +_serv_name + "\r\n";
      _pfd.events |= POLLOUT;
      _ping_time = curr_time;
    }

    if (ready < 0) {
      if (errno == EINTR) continue;
      throw std::runtime_error("poll faild");
    }

    if (_pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
      break;
    } else {
      if (_pfd.revents & POLLIN) {
        if (read_income_msg() == -1) break;
      } else if (_pfd.revents & POLLOUT) {
        send_message_to_server();
      }
    }
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

    _recv_buffer.append(buf, bytes);

    size_t pos;
    while ((pos = _recv_buffer.find("\n")) != std::string::npos) {
      std::string line = _recv_buffer.substr(0, pos);
      _recv_buffer.erase(0, pos + 1);

      if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

      handleLine(line);
    }
  }
  return 0;
}

void Bot::send_message_to_server() {
  while (!_send_buffer.empty()) {

    ssize_t  n = send(_connected_fd, _send_buffer.c_str(), _send_buffer.size(), 0);
    if (n < 0) break;

    if (static_cast<size_t>(n) < _send_buffer.size()) {
      _send_buffer.erase(0, n);
      break;
    }
    
    _send_buffer.erase(0, n);
  }

  _pfd.events = POLLIN;
  if (!_send_buffer.empty())  _pfd.events |= POLLOUT;
}

std::map<std::string, std::string> Bot::parse_incomming_msg(const std::string &str) {
  std::map<std::string, std::string> t;
  std::stringstream ss(str);
  std::string s;
  bool flag = false;

  while (ss >> s) {
    if (s[0] == '#'|| s[0] == '&')
      t["channel"] = s;
    else if (s.find(":") != std::string::npos && !flag) {
      int end = 0;
      for (size_t i = 1; i < s.size(); i++) {
        if (s[i + 1] == '!' || s[i + 1] == ' ' || s[i + 1] == '\0') {
          end = i;
          break;
        }
      }
      t["receiver"] =  str.substr(1, end);
      flag = true;
    } else if (s == "INVITE" || s == "PRIVMSG") {
      t["cmd"] = s;
    } else if (s.find(":") != std::string::npos && flag) {
      t["msg"] = s.substr(s.find(" :") + 2);
      flag = false;
    }
  }
  return t;
}

const std::string &Bot::get_param(std::map<std::string, std::string> &t, const std::string &str) {
    std::map<std::string, std::string>::iterator it = t.begin();
    for (; it != t.end(); ++it) {
      if (it->first == str) return it->second;
    }
    return str;
}


void Bot::handleLine(const std::string &line) {
    std::cout << "[IRC] " << line << std::endl;

    std::map<std::string, std::string> t = parse_incomming_msg(line);
    std::string cmd = get_param(t, "cmd");
    std::string channel = get_param(t, "channel");

    if (line.find(" 001 ") != std::string::npos) {
      std::string receiver = get_param(t, "receiver");
      _serv_name = receiver;
    }
    
    if (cmd == "INVITE") {
      if (channel.empty()) return ;

      _send_buffer += "JOIN " + channel + "\r\n";
      _send_buffer += "PRIVMSG " + channel + " :Hello 👋 I am an irc bot\r\n";

      _pfd.events |= POLLOUT;
      return;
    }

    if (cmd == "PRIVMSG") {
      std::string receiver = get_param(t, "receiver");
      std::string msg = get_param(t, "msg");

      std::string target = (channel == "channel") ? receiver : channel;

      if (msg == "hello" || msg == "hi") 
      {
        _send_buffer += "PRIVMSG " + target + " :Hi there 👋, I am an IRC bot\r\n";
      } 
      else if (msg == "time") 
      {
        time_t t = time(NULL);
        std::string time = std::string(ctime(&t));
        time.erase(time.size() - 1);

        _send_buffer += "PRIVMSG " + target + " :" + time + "\r\n";
      }
      else if (msg == "joke" || msg == "fun")
      {
        std::string t = "Five out of six people agree that Russian Roulette is safe. 😁";
        _send_buffer += "PRIVMSG " + target + " :" + t + "\r\n";
      }

      t.clear();
      _pfd.events |= POLLOUT;
    }
}

Bot::~Bot() {
  if (_connected_fd != -1)
    close(_connected_fd);
}