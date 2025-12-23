#include "Bot.hpp"

sig_atomic_t signaled = 1;

Bot::Bot(const std::string &ip, const std::string &port, const std::string &password) : _ip(ip),  _port(port), 
  _password(password), _nick("ircBot"), _connected_fd(-1), _ping_time(time(NULL)) {

  _connected_fd = getsocketfd();
  if (_connected_fd == -1) throw std::runtime_error("Error: connection");

  if (fcntl(_connected_fd, F_SETFL, O_NONBLOCK) == -1)
    throw std::runtime_error("faild to make non-blocking mode");

  int res = connect(_connected_fd, (struct sockaddr *)&_serv_addr, sizeof(_serv_addr));
  if ( res < 0 && errno != EINPROGRESS) {
    close(_connected_fd);
    throw std::runtime_error("Connection Failed");
  }

  int err;
  socklen_t len = sizeof(err);
  getsockopt(_connected_fd, SOL_SOCKET, SO_ERROR, &err, &len);
  if (err != 0) {
    close(_connected_fd);
    throw std::runtime_error("Connection Failed");
  }

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

void Bot::run() 
{ 
    signal(SIGQUIT, SIG_IGN);
    signal(SIGINT, stop_listen);

    if (!_password.empty()) {
        std::string pass = "PASS " + _password + "\r\n"; 
        _send_buffer.append(pass + "NICK " + _nick + "\r\nUSER " + _nick + " * 0 :" + _nick + "\r\n");
        _pfd.events |= POLLOUT;
    }
  
    while (signaled) 
    {
        int ready = poll(&_pfd, 1, 1000);
        if (ready < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll faild");
        }

        ping();

        if (_pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            break;
        } 
        else 
        {
            if (_pfd.revents & POLLIN) 
            {
                if (read_income_msg() == -1) break;
            } 
            else if (_pfd.revents & POLLOUT) 
            {
                send_message_to_server();
            }
        }
    }
}

int Bot::read_income_msg() {
    char buf[512];

    while (true) 
    {
        ssize_t bytes = recv(_connected_fd, buf, sizeof(buf), 0);
        if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (bytes == 0) return -1;

        _recv_buffer.append(buf, bytes);

		if (_recv_buffer.size() > 512) return -1;

        size_t pos;
        while ((pos = _recv_buffer.find("\r\n")) != std::string::npos) 
        {
            std::string line = _recv_buffer.substr(0, pos);
            _recv_buffer.erase(0, pos + 2);

            handleLine(line);
        }
    }
    return 0;
}

void Bot::send_message_to_server() 
{
    while (!_send_buffer.empty()) 
    {
        ssize_t  n = send(_connected_fd, _send_buffer.c_str(), _send_buffer.size(), 0);
        if (n < 0) break;

        if (static_cast<size_t>(n) < _send_buffer.size()) 
        {
            _send_buffer.erase(0, n);
            break;
        }
        _send_buffer.erase(0, n);
    }

    _pfd.events = POLLIN;
    if (!_send_buffer.empty())  _pfd.events |= POLLOUT;
}

std::map<std::string, std::string>
Bot::parse_incoming_msg(const std::string &line)
{
    std::map<std::string, std::string> res;

    if (line.empty()) return res;

    size_t pos = 0;
    if (line[pos] == ':') 
	{
        size_t end = line.find(' ');
        if (end == std::string::npos) return res;

        res["prefix"] = line.substr(1, end - 1);

        if (line.find('!') != std::string::npos && line.find('@') != std::string::npos)
		{
            size_t e = line.find('!');
            if (e == std::string::npos) return res;
			res["sender"] = line.substr(1, e - 1);
		}
        pos = end + 1;
    } 
    else 
    {
        if (line.find("PONG") != std::string::npos)
        {
            size_t end = line.find(" ");
            if (end != std::string::npos) 
            {
                res["cmd"] = line.substr(pos, end);
                res["sender"] = line.substr(end + 1);
                return res;
            }
        }
        res.clear();
        return res;
    }

    size_t end = line.find(' ', pos);
    if (end == std::string::npos) 
	{
        res["cmd"] = line.substr(pos);
        return res;
    }

    res["cmd"] = line.substr(pos, end - pos);
    pos = end + 1;

    while (pos < line.size()) 
	{
        if (line[pos] == ':') 
		{
            res["msg"] = line.substr(pos + 1);
            break;
        }

        end = line.find(' ', pos);
        if (end == std::string::npos)
            end = line.size();

        std::string param = line.substr(pos, end - pos);

        if (param[0] == '#' || param[0] == '&')
            res["channel"] = param;

        pos = end + 1;
    }
    return res;
}


const std::string &Bot::get_param(std::map<std::string, std::string> &t, const std::string &str) {
    std::map<std::string, std::string>::iterator it = t.begin();
    for (; it != t.end(); ++it) {
      if (it->first == str) return it->second;
    }
    return str;
}


void Bot::handleLine(const std::string &line) 
{
    std::map<std::string, std::string> t = parse_incoming_msg(line);

	std::string cmd     = get_param(t, "cmd");
	std::string channel = get_param(t, "channel");
	std::string msg     = get_param(t, "msg");
	std::string prefix  = get_param(t, "prefix");
	std::string sender  = get_param(t, "sender");

    if (cmd == "001") _serv_name = prefix;
    else if (cmd == "INVITE") invite(channel);
    else if (cmd == "PRIVMSG") {
        std::string target = (channel == "channel") ? sender : channel;
        privmsg(target, msg);
    } else if (cmd == "PONG") pong(sender);

    t.clear();
}

void Bot::invite(const std::string &channel) {
    if (channel == "channel") return ;
    _send_buffer += "JOIN " + channel + "\r\n";
    _send_buffer += "PRIVMSG " + channel + " :Hello 👋 I am an irc bot\r\n";

    _pfd.events |= POLLOUT;
}

void Bot::privmsg(const std::string &target, const std::string &msg) {
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
    _pfd.events |= POLLOUT;
}

void Bot::pong(const std::string &sender) 
{
    if (_serv_name.empty() || sender != _serv_name) 
        raise(2);
        
    time_t t = time(NULL);
    if (t - _ping_time > _pong_recv_time)
        raise(2);
}

void Bot::ping() {
    int curr_time = time(NULL);
    if (curr_time - _ping_time > _ping_wind) 
    {
        _send_buffer += "PING " +_serv_name + "\r\n";
        _pfd.events |= POLLOUT;
        _ping_time = curr_time;
    }
}

Bot::~Bot() 
{
    if (_connected_fd != -1)
        close(_connected_fd);
}