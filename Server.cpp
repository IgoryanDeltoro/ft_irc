#include "Server.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <string>

Server::Server(const std::string &port, const std::string &password) : _listen_fd(-1), _port(port), _password(password) {
    _listen_fd = create_and_bind();
    if (_listen_fd < 0) throw std::runtime_error("Failed to bind listening socket");
    if (listen(_listen_fd, BECKLOG) < 0)
    {
        close(_listen_fd);
        throw std::runtime_error("listen() failed");
    }

    //make non-blocking
    // int flags = fcntl(_listen_fd, F_GETFL, O_NONBLOCK);
    // if (flags == -1) throw std::runtime_error("faild to make non-blocking mode");
    if (fcntl(_listen_fd, F_SETFL, O_NONBLOCK) == -1) throw std::runtime_error("faild to make non-blocking mode");

    struct pollfd p;
    p.fd = _listen_fd;
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);
}

Server::~Server() {
    std::map<int, Client*>::iterator it = _clients.begin();
    for (; it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    if (_listen_fd >= 0) close(_listen_fd);
    
    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        delete it->second;
    }
}

int Server::create_and_bind() {
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, _port.c_str(), &hints, &res) != 0) {
        std::cout << "Error: getaddrinfo" << std::endl;
        return -1;
    }

    int server_fd = -1;
    for (struct addrinfo *ad = res; ad != NULL; ad = ad->ai_next) {
        server_fd = socket(ad->ai_family, ad->ai_socktype, ad->ai_protocol);
        if (server_fd < 0) continue;

        // Attach socket to the port and reconnect
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Bind
        if (bind(server_fd, ad->ai_addr, ad->ai_addrlen) == 0) break;
        close(server_fd);
        server_fd = -1;
    }
    freeaddrinfo(res);
    return server_fd;
}

void Server::run() {
    std::cout << "Listening on port: " << _port << std::endl;

    while (1) {
        int ready = poll(&_pfds[0], _pfds.size(), 1000);
        if (ready < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll faild");
        }

        /* Deal with array returned by poll(). */
        for (size_t j = 0; j < _pfds.size(); j++) {
            struct pollfd p = _pfds[j]; 
            if (p.revents == 0) continue;
            if (p.fd == _listen_fd && (p.revents & POLLIN)) {
                eccept_new_fd(); // rewrite below code in another function
            } else {
                std::map<int, Client*>::iterator it = _clients.find(p.fd); 
                if (it == _clients.end()) continue;
                Client *c = it->second;

                if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    std::cout << "Clean filedescriptors\n";
                    clean_fd(p.fd);
                } else {
                    if (p.revents & POLLIN) read_message_from(c);
                    if (p.revents & POLLOUT) send_msg_to(c);
                }
            }
            p.revents = 0;
        }
    }
}

void Server::eccept_new_fd() {
    // Accept a connection
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    while (1) {
        int new_fd = accept(_listen_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_fd < 0) {
            if (errno == (EAGAIN | EWOULDBLOCK)) break;
            else break;
        }

        // make non-blocking mode
        if (fcntl(new_fd, F_SETFL, O_NONBLOCK) == -1) {
            close(new_fd);
            throw std::runtime_error("faild to make non-blocking mode");
        }

        _clients[new_fd] = new Client(new_fd);
    
        struct pollfd pa;
        pa.fd = new_fd;
        pa.events = POLLIN;
        pa.revents = 0;
        _pfds.push_back(pa);

        std::cout << "Accepted fd=" << new_fd << std::endl;
        send_message_to_client(new_fd, "Welcome to IRC chat.\r\n");
    }
}

void Server::read_message_from(Client *c) {
    if (!c) throw std::runtime_error("while reading incoming call");

    char buff[BUFFER] = {0};

    while (1) {
        ssize_t  bytes = recv(c->getFD(), buff, sizeof(buff), 0);

        if (bytes > 0) {
            c->getRecvBuff().append(buff, buff + bytes);

            // split lines on CRLF or LF
            size_t pos;
            while ((pos = c->getRecvBuff().find('\n')) != std::string::npos) {
                std::string line = c->getRecvBuff().substr(0, pos);
                c->getRecvBuff().erase(0, pos + 1); // erase buffer till LF ('\n')

                // strip CR if present
                // if last element CR then delete it
                if (!line.empty() && line[line.size() - 1] == '\r') { 
                    line.erase(line.size() - 1);
                }
                // handle process line
                process_line(c, line); 
            }
        } else if (bytes == 0) {
            // clean file descriptors
            clean_fd(c->getFD());
            break;
        } else {
            if (errno & (EAGAIN | EWOULDBLOCK)) break;
            // clean file descriptors
            clean_fd(c->getFD());
            break;
        }
    }
}

ssize_t Server::send_message_to_client(int fd, std::string msg) {
    if (fd < 0) return -1;
    return send(fd, msg.c_str(), msg.length(), 0);
}

void Server::send_msg_to(Client *c) {
    while (!c->getMessage().empty()) {
        std::string s = c->getMessage().front();
        c->getMessage().pop_front();
        ssize_t n = send_message_to_client(c->getFD(), s);
        if (n < 0) {
            if (errno & (EAGAIN | EWOULDBLOCK)) {
                break;
            } else {
                clean_fd(c->getFD());
                return;
            }
        }
        if (static_cast<size_t>(n) < s.size()) {
            s.erase(0, n);
            break;
        }

        // set poll events
        for (size_t i = 0; i < _pfds.size(); i++) {
            if (c->getFD() == _pfds[i].fd) {
                _pfds[i].events = POLLIN;
                if (!c->getMessage().empty()) _pfds[i].events |= POLLOUT;
                break;
            }
        }
    }
}

void Server::clean_fd(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return ;

    std::cout << "User " << it->second->getNick() << " leave session" << std::endl;
    
    // don't foget to remove from channels

    close(it->first);
    delete it->second;
    _clients.erase(it);

    // remove from _pfds
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds.erase(_pfds.begin() + i);
            break;
        }
    }

}

void Server::pass(Client *c, const Command &command)
{
    std::vector<std::string> params = _parser.splitParams(command.getMode());

    if (params.size() != 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "PASS");
        return;
    }
    if (c->getPassStatus())
    {
        sendError(c, ERR_PASSALREADY, "");
        return;
    }
    if (params[0] == _password)
        c->setPassStatus(true);
    else
    {
        sendError(c, ERR_INCORRECTPASSWORD, "");
        return;
    }
}

bool Server::isNickExists(const std::string &nick, Client *client)
{
    for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client *c = it->second;
        if (c != client && c->getNick() == nick)
            return true;
    }
    return false;
}

void Server::nick(Client *c, const Command &command)
{
    if (!c->getPassStatus())
    {
        sendError(c, ERR_NEEDPASS, "");
        return;
    }

    std::vector<std::string> params = _parser.splitParams(command.getMode());
    if (params.size() < 1)
    {
        sendError(c, ERR_NONICKNAMEGIVEN, "");
        return;
    }

    std::string newNick = params[0];
    if (!_parser.isValidNick(newNick))
    {
        sendError(c, ERR_ERRONEUSNICKNAME, newNick);
        return;
    }
    if (isNickExists(newNick, c))
    {
        sendError(c, ERR_NICKNAMEINUSE, newNick);
        return;
    }

    std::string oldNick = c->getNick();
    c->setNick(newNick);

    if (oldNick.empty())
    {
        std::cout << "User assigned nick: " << newNick << std::endl;

        if (!c->getNick().empty() && !c->getUserName().empty() && !c->getRealName().empty()&&  c->getPassStatus())
        {
            c->setRegStatus(true);
            send_message_to_client(c->getFD(), "User is REGISTERED.\r\n");
        }
        return;
    }
    else
    {
        std::string msg = ":" + oldNick + " NICK " + newNick + "\r\n";
        std::cout << msg;
    }
}

void Server::user(Client *c, const Command &command)
{
    if (!c->getPassStatus())
    {
        sendError(c, ERR_NEEDPASS, "");
        return;
    }

    std::vector<std::string> params = _parser.splitParams(command.getMode());
    std::cout << "USER Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.size() < 3 || command.getText().empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "USER");
        return;
    }

    std::string userName = params[0];
    _parser.trim(userName);
     if (userName.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "USER");
        return;
    }

    std::string realName = command.getText();
    _parser.trim(realName);
    if (realName.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "USER");
        return;
    }
    if (c->getRegStatus())
    {
        sendError(c, ERR_ALREADYREGISTRED, "");
        return;
    }

    c->setUserName(userName);
    c->setRealName(realName);

    if (!c->getNick().empty() && !c->getUserName().empty() && !c->getRealName().empty()&&  c->getPassStatus())
    {
        c->setRegStatus(true);
        send_message_to_client(c->getFD(), "User is REGISTERED.\r\n");
    }
}

void Server::join(Client *c, const Command &command)
{
    if (!c->getPassStatus())
    {
        sendError(c, ERR_NEEDPASS, "");
        return;
    }
    if (!c->getRegStatus())
    {
        sendError(c, ERR_NOTREGISTERED, "");
        return;
    }

    std::vector<std::string> params = _parser.splitParams(command.getMode());
    std::cout << "JOIN Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "JOIN");
        return;
    }

    
    // JOIN 0 → leave all channels
    // if (params[0] == "0")
    // {
    //     leaveAllChannels(c);
    //     return;
    // }

    // channels (#a,#b,#c)
    std::vector<std::string> channelNames = _parser.splitByComma(params[0]);
    std::vector<std::string> keys;

    if (params.size() > 1)
    {
        keys = _parser.splitByComma(params[1]);
    }

    for (size_t i = 0; i < channelNames.size(); ++i)
    {
        std::string chan = channelNames[i];
        std::string key = (i < keys.size() ? keys[i] : "");

        joinChannel(c, chan, key);
    }

    //    If the channel doesn't exist prior to joining,
    // the channel is created and the creating user becomes a
    // channel operator. If the channel already exists, whether or not your request to JOIN that channel is honoured depends on the current modes of the channel. For example, if the channel is invite-only, (+i) then you may only join if invited. As part of the protocol, a user
    // may be a part of several channels at once, but a limit of ten (10)
    // channels is recommended as being ample for both experienced and
    // novice users. See section 8.13 for more information on this.

    // The JOIN command is used by client to start listening a specific channel.Whether or  not a client is allowed to join a channel is checked only by the server the client is connected to;
    // all other servers automatically add the user to the channel when it is received from other servers.The conditions which affect this are as follows :

    //     1. the user must be invited if the channel is invite -
    //     only;
    // 2. the user's nick/username/hostname must not match any active bans;

    //  3. the correct key (password) must be given if it is set.

    //  These are discussed in more detail under the MODE command (see
    //  section 4.2.3 for more details).

    //  Once a user has joined a channel, they receive notice about all
    //  commands their server receives which affect the channel. This
    //  includes MODE, KICK, PART, QUIT and of course PRIVMSG/NOTICE. The
    //  JOIN command needs to be broadcast to all servers so that each server
    //  knows where to find the users who are on the channel. This allows
    //  optimal delivery of PRIVMSG/NOTICE messages to the channel.

    //  If a JOIN is successful, the user is then sent the channel's topic
    //  (using RPL_TOPIC) and the list of users who are on the channel (using
    //  RPL_NAMREPLY), which must include the user joining.

    //  Numeric Replies:

    //  ERR_NEEDMOREPARAMS ERR_BANNEDFROMCHAN
    //  ERR_INVITEONLYCHAN ERR_BADCHANNELKEY
    //  ERR_CHANNELISFULL ERR_BADCHANMASK
    //  ERR_NOSUCHCHANNEL ERR_TOOMANYCHANNELS
    //  RPL_TOPIC

    //  Examples:

    //  JOIN #foobar ;
    //  join channel #foobar.

    //  JOIN &foo fubar;
    //  join channel &foo using key "fubar".

    //  JOIN #foo, &bar fubar;
    //  join channel #foo using key "fubar" and &bar using no key.

    //  JOIN #foo, #bar fubar, foobar;
    //  join channel #foo using key "fubar". and channel #bar using key "foobar".

    //  JOIN #foo, #bar;
    //  join channels #foo and#bar.

    //  : WiZ JOIN #Twilight_zone;
    //  JOIN message from WiZ
}


void Server::joinChannel(Client *c, const std::string &name, const std::string &password)
{
    //проверить имя на валидность!

    Channel *ch;

    if (_channels.count(name) == 0)
    {
        send_message_to_client(c->getFD(), ": Channel " + name + " not found \r\n");

        ch = new Channel(name, c);
        _channels[name] = ch;
        ch->addUser(c);
        ch->addOperator(c->getFD());
        send_message_to_client(c->getFD(), ": Channel " + name + " added with new user as operator\r\n");
        return;
    }
    else
    {
        ch = _channels[name];
        send_message_to_client(c->getFD(), ": Channel " + name + " found \r\n");

        // +i (invite only)
        if (ch->isI() && ch->getInvited().count(c->getFD()) == 0)
        {
            sendError(c, ERR_INVITEONLYCHAN, name);
            return;
        }

        // +k (password)
        if (!ch->getPassword().empty() && ch->getPassword() != password)
        {
            sendError(c, ERR_BADCHANNELKEY, name);
            return;
        }

        // +l (user limit)
        if (ch->getUserLimit() > 0 && (int)ch->getUsers().size() >= ch->getUserLimit())
        {
            sendError(c, ERR_CHANNELISFULL, name);
            return;
        }

        // пользователь есть в канале? - пропустить

        ch->addUser(c);
        send_message_to_client(c->getFD(), ": Channel " + name + " USER ADDED\r\n");
    }
}

void Server::mode(Client *c, const Command &command)
{
    (void)c;
    (void)command;
    //     MODE #Finnish + im;
    //     Makes #Finnish channel moderated and 'invite-only'.

    //      MODE #Finnish +o Kilroy;
    //     Gives 'chanop' privileges to Kilroy on channel #Finnish.

    //         MODE #Finnish +
    //         v Wiz;
    //     Allow WiZ to speak on #Finnish.

    //         MODE #Fins -
    //         s;
    //     Removes 'secret' flag from channel
    // #Fins.

    //             MODE #42 +
    //         k oulu;
    //     Set the channel key to "oulu".

    //         MODE #eu -
    //         opers + l 10; Set the limit for the number of users
    //      on channel to 10.

    //      MODE &oulu +b ; list ban masks set for channel.

    //      MODE &oulu +b *!*@* ;
    //     prevent all users from joining.

    //         MODE &oulu +
    //         b * !*@ *.edu;
    //     prevent any user from a hostname matching *.edu from joining.
}

void Server::invite(Client *c, const Command &command)
{
    (void)c;
    (void)command;
    //             The INVITE message is used to invite users to a channel.The
    //                 parameter<nickname>
    //                     is the nickname of the person to be invited to
    //                         the target channel<channel>
    //                             .There is no requirement that the
    //                                 channel the target user is being invited to must exist or
    //         be a valid
    //             channel.To invite a user to a channel which is invite only(MODE + i),
    //         the client sending the invite must be recognised as being a
    //             channel
    //             operator on the given channel.

    //         Numeric Replies :

    //         ERR_NEEDMOREPARAMS ERR_NOSUCHNICK
    //             ERR_NOTONCHANNEL ERR_USERONCHANNEL
    //                 ERR_CHANOPRIVSNEEDED
    //                     RPL_INVITING RPL_AWAY

    //         Examples :
    //         : Angel INVITE Wiz #Dust; User Angel inviting WiZ to channel #Dust
    //         INVITE Wiz #Twilight_Zone; Command to invite WiZ to #Twilight_zone
}

void Server::kick(Client *c, const Command &command)
{
    (void)c;
    (void)command;
    // Only a channel operator may kick another user out of a channel.Each server that receives a KICK message checks that it is valid(ie the sender is actually a channel operator) before removing
    // the victim from the channel.

    //     Numeric Replies :

    //     ERR_NEEDMOREPARAMS ERR_NOSUCHCHANNEL
    //         ERR_BADCHANMASK ERR_CHANOPRIVSNEEDED
    //             ERR_NOTONCHANNEL

    //                 Examples :

    // KICK &Melbourne Matthew; Kick Matthew from &Melbourne

    // KICK #Finnish John : Speaking English; Kick John from #Finnish using "Speaking English" as the reason(comment).

    // : WiZ KICK #Finnish John; KICK message from WiZ to remove John from channel #Finnish

    //  NOTE : It is possible to extend the KICK command parameters to the following :
    // <channel>{, <channel>} < user>{, <user>}[<comment>]
}

void Server::help(Client *c)
{
    std::string pass = "PASS <password>\n";
    std::string nick = "NICK <nickname>\n";
    std::string user = "USER <username> <hostname> <servername> :<realname>\n";
    std::string join = "JOIN <channel>{,<channel>} [<key>{,<key>}]\nPART\n";
    std::string mode = "MODE <channel> {[+|-]|o|p|s|i|t|n|b|v} [<limit>] [<user>][<ban mask>]\n";
    std::string topic = "TOPIC <channel> [<topic>]\n";
    std::string list = "LIST <channel> [<topic>]\n";
    std::string invite = "INVITE <nickname> <channel>\n";
    std::string kick = "KICK <channel> <user> [<comment>]\n";
    std::string privmsg = "PRIVMSG <receiver>{,<receiver>} :<text to be sent>\n";

    if (!c->getRegStatus())
        send_message_to_client(c->getFD(), pass + nick + user + "\r\n");
    else
        send_message_to_client(c->getFD(), join + mode + topic + list + invite + kick + privmsg + "\r\n");
}

void Server::topic(Client *c, const Command &command)
{
    if (!c->getPassStatus())
    {
        sendError(c, ERR_NEEDPASS, "");
        return;
    }
    if (!c->getRegStatus())
    {
        sendError(c, ERR_NOTREGISTERED, "");
        return;
    }
    std::vector<std::string> params = _parser.splitParams(command.getMode());
    std::cout << "TOPIC Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.size() != 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "TOPIC");
        return;
    }
    std::string chanelName = params[0];
    //check chanelName

    Channel *ch;
    if (_channels.count(chanelName) == 0)
    {
        // sendError(c, ERR_NEEDMOREPARAMS, "TOPIC");
        send_message_to_client(c->getFD(), ":CHANEL NOT FOUND\r\n");
        return;
    }
    else
        ch = _channels[chanelName];

    if (command.getText().empty())
    {
        std::string topicToSend = ch->getTopic().empty() ? "Topic not set" : ch->getTopic();
        send_message_to_client(c->getFD(), ":" + topicToSend + "\r\n");
        return;
    }
    else
    {
        //check user operator and chanel can change topic
        //change channel topic
    }
}

void Server::cap(Client *c, const Command &command)
{
    std::vector<std::string> params = _parser.splitParams(command.getMode());
    if (params.size() == 0) return;
    if (params[0] == "LS") send_message_to_client(c->getFD(), ":server CAP * LS :\r\n");
    // else if (params[0] == "END") {}
}

void Server::sendError(Client *c, Error err, const std::string &arg)
{
    std::string message = getErrorText(err);
    std::string nick = c->getNick().empty() ? "*" : c->getNick();

    size_t pos;

    if ((pos = message.find("<command>")) != std::string::npos)
        message.replace(pos, 9, arg);
    if ((pos = message.find("<nick>")) != std::string::npos)
        message.replace(pos, 6, arg);
    if ((pos = message.find("<channel>")) != std::string::npos)
        message.replace(pos, 9, arg);


    std::string s;
    std::stringstream out1;
    out1 << err;
    s = out1.str();

    std::string out = ":server " + s + " " + nick + " " + message + "\r\n";
    send_message_to_client(c->getFD(), out);
}

std::string Server::getErrorText(const Error &error)
{
    switch (error)
    {
        // case :
        //     return ""
        // case :
        //     return ""
        // case :
        //     return ""
        // case :
        //     return ""
    case ERR_NOTREGISTERED:
        return ": User has not registration";
    case ERR_BANNEDFROMCHAN:
        return "<channel> :Cannot join channel (+b)";
    case ERR_INVITEONLYCHAN:
        return "<channel> :Cannot join channel (+i)";
    case ERR_BADCHANNELKEY:
        return "<channel> :Cannot join channel (+k)";
    case ERR_CHANNELISFULL:
        return "<channel> :Cannot join channel (+l)";
    case ERR_BADCHANMASK:
        return "";
    case ERR_NOSUCHCHANNEL:
        return "<channel> :No such channel";
    case ERR_TOOMANYCHANNELS:
        return "<channel> :You have joined too many channels";
    case RPL_TOPIC:
        return "<channel> :<topic>";
    case ERR_PASSALREADY:
        return ":Password already success";
    case ERR_NEEDPASS:
        return ":Server PASS required ";
    case ERR_INCORRECTPASSWORD:
        return ":Wrong password";
    case ERR_ALREADYREGISTRED:
        return ":You may not reregister";
    case ERR_NEEDMOREPARAMS:
        return "<command> :Not enough parameters";
    case ERR_NONICKNAMEGIVEN:
        return ":No nickname given";
    case ERR_ERRONEUSNICKNAME:
        return "<nick> :Erroneus nickname";
    case ERR_NICKNAMEINUSE:
        return "<nick> :Nickname is already in use";
    default:
        return ":Error";
    }
}

void Server::process_line(Client *c, std::string line)
{
    std::cout << "c->buff: " << c->getRecvBuff() << std::endl;
    std::cout << "line: " << line << std::endl;
    _parser.trim(line);
    if (line.empty())
    {
        return;
        Command(NOT_FOUND, "", ""); //???
    }

    Command cmnd = _parser.parse(*c, line);

    if (cmnd.getCommand() == NOT_FOUND)
        send_message_to_client(c->getFD(), ":Command not found <" + line + ">\r\n");
    else if (cmnd.getCommand() == HELP)
        help(c);
    else if (cmnd.getCommand() == PASS)
        pass(c, cmnd);
    else if (cmnd.getCommand() == NICK)
        nick(c, cmnd);
    else if (cmnd.getCommand() == USER)
        user(c, cmnd);
    else if (cmnd.getCommand() == JOIN)
        join(c, cmnd);
    else if (cmnd.getCommand() == MODE)
        mode(c, cmnd);
    else if (cmnd.getCommand() == KICK)
        kick(c, cmnd);
    else if (cmnd.getCommand() == TOPIC)
        topic(c, cmnd);
    else if (cmnd.getCommand() == INVITE)
        invite(c, cmnd);
    else if (cmnd.getCommand() == CAP)
        cap(c, cmnd);
    else if (cmnd.getCommand() == PRIVMSG)
        privmsg(c, cmnd);
}

void Server::handlePRIVMSG(Client *c, const std::string &target, const std::string &message)
{
    if (target.empty()) return;

    std::map<std::string, Client*>::iterator it = _nicks.find(target);

    if (it == _nicks.end()) return;
    it->second->enqueue_reply(std::string(":") + (c->getNick().empty() ? "*" : c->getNick()) + " PRIVMSG " + target + " :" + message + "\r\n");
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == it->second->getFD()) {
            _pfds[i].events |= POLLOUT;
            break;
        }
    }
}

void Server::privmsg(Client *c, const Command &cmd)
{
    (void)c;
    (void)cmd;
}