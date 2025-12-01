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
#include <iostream>

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
        _clients[new_fd]->enqueue_reply("Welcome to IRC chat.\r\n");
        set_event_for_sending_msg(_clients[new_fd]->getFD());
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
        ssize_t n = send(c->getFD(), s.c_str(), s.length(), 0);
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
                else _pfds[i].events &= ~POLLOUT;
                break;
            }
        }
    }
}

void Server::clean_fd(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return ;

    std::string def_nick = it->second->getNick().empty() ? "*_" : it->second->getNick();
    std::cout << GREEN "User " << def_nick << " leave session" RESET << std::endl;
    
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

    // if (!c->getNick().empty() && !c->getUserName().empty() && !c->getRealName().empty())
    // {
    //     c->setRegStatus(true);
    //     c->enqueue_reply(GREEN "User is REGISTERED" RESET "\r\n");
    //     set_event_for_sending_msg(c->getFD());
    // }
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
    _nicks[newNick] = c;

    if (oldNick.empty())
    {
        std::cout << "User assigned nick: " << newNick << std::endl;

        if (!c->getNick().empty() && !c->getUserName().empty() && !c->getRealName().empty() && c->getPassStatus())
        {
            c->setRegStatus(true);
            sendWelcome(c);
        }
        return;
    }
    else
    {
        std::string msg = ":" + oldNick + " NICK " + newNick + "\r\n";
        std::cout << msg;


        // Разошли клиенту, и всем в его каналах:
        // c->enqueue_reply(msg);
        // std::map<std::string, Channel*>::iterator it = _channels.begin();
        // for (; it != _channels.end(); ++it) {
        //     if (it->second->isUser(c))
        //         it->second->broadcast(c, msg); // если реализовано
        // }        
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

        sendWelcome(c);
        // c->enqueue_reply(GREEN "User is REGISTERED" RESET "\r\n");
        // set_event_for_sending_msg(c->getFD());
    }
}

void Server::join(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
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

    // trim params[0] ?
    std::vector<std::string> channelNames = _parser.splitByComma(params[0]);
    std::vector<std::string> keys;

    if (params.size() > 1)
    {
        // trim params[1]
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
        ch = new Channel(name, c);
        _channels[name] = ch;
        ch->addUser(c);
        ch->addOperator(c->getNick());
        send_message_to_client(c->getFD(), ": Channel " + name + " added with new user as operator\r\n");
        return;
    }
    else
    {
        ch = _channels[name];
        // +i (invite only)
        if (ch->isI() && ch->getInvited().count(c->getNick()) == 0)
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

        const std::set<std::string> &users = ch->getUsers(); // предполагаем getUsers() const -> const&
        std::string nick = c->getNick();
        if (users.find(nick) != users.end())
        {
            send_message_to_client(c->getFD(), ":Channel " + name + " USER already in channel\r\n");
            return;
        }
        ch->addUser(c);
        send_message_to_client(c->getFD(), ":Channel " + name + " USER ADDED\r\n");
    }
}

void Server::mode(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
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

bool Server::isClientAuth(Client *client)
{

    if (!client->getPassStatus())
    {
        sendError(client, ERR_NEEDPASS, "");
        return false;
    }
    if (!client->getRegStatus())
    {
        sendError(client, ERR_NOTREGISTERED, "");
        return false;
    }
    return true;
}

void Server::invite(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;

        //             The INVITE message is used to invite users to a channel.The parameter<nickname>
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

        //         ERR_NOSUCHNICK
        //         ERR_NOTONCHANNEL ERR_USERONCHANNEL
        //         ERR_CHANOPRIVSNEEDED
        //         RPL_INVITING RPL_AWAY

    std::vector<std::string> params = _parser.splitParams(command.getMode());
    std::cout << "JOIN Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.size() < 2)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "JOIN");
        return;
    }
    // <nickname><channel>
    std::string nick = params[0];
    std::string channel = params[1];

    // check NIC CHAN


    //проверить ник в базе

    Channel *ch;
    if (_channels.count(channel) == 0)
    {
        sendError(c, ERR_NOTONCHANNEL, channel);
        return;
    }
    ch = _channels[channel];

    if (!ch->isUser(c->getNick()))
    {
        sendError(c, ERR_NOTONCHANNEL, channel);
        return;
    }

     // +i (invite only)
    if (ch->isI() && ch->getOperators().count(c->getNick()) == 0)
    {
        sendError(c, ERR_INVITEONLYCHAN, channel);
        return;
    }

    //     // +k (password)
    //     if (!ch->getPassword().empty() && ch->getPassword() != password)
    //     {
    //         sendError(c, ERR_BADCHANNELKEY, name);
    //         return;
    //     }

    //     // +l (user limit)
    //     if (ch->getUserLimit() > 0 && (int)ch->getUsers().size() >= ch->getUserLimit())
    //     {
    //         sendError(c, ERR_CHANNELISFULL, name);
    //         return;
    //     }

    //     const std::map<int, Client *> &users = ch->getUsers(); // предполагаем getUsers() const -> const&
    //     int fd = c->getFD();
    //     if (users.find(fd) != users.end())
    //     {
    //         send_message_to_client(fd, ":Channel " + name + " USER already in channel\r\n");
    //         return;
    //     }
    //     ch->addUser(c);
    //     send_message_to_client(c->getFD(), ":Channel " + name + " USER ADDED\r\n");
    

    //         Examples :
    //         : Angel INVITE Wiz #Dust; User Angel inviting WiZ to channel #Dust
    //         INVITE Wiz #Twilight_Zone; Command to invite WiZ to #Twilight_zone

    ch->addInvite(nick);
}

void Server::kick(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;

    //     ERR_NEEDMOREPARAMS ERR_NOSUCHCHANNEL
    //         ERR_BADCHANMASK ERR_CHANOPRIVSNEEDED
    //             ERR_NOTONCHANNEL

    //                 Examples :

    // KICK &Melbourne Matthew; Kick Matthew from &Melbourne
    // KICK #Finnish John : Speaking English; Kick John from #Finnish using "Speaking English" as the reason(comment).
    // : WiZ KICK #Finnish John; KICK message from WiZ to remove John from channel #Finnish
    //  NOTE : It is possible to extend the KICK command parameters to the following :
    // <channel> <user> [<comment>]
    // <channel>{, <channel>} < user>{, <user>}[<comment>]

    std::vector<std::string> params = _parser.splitParams(command.getMode());
    std::cout << "JOIN Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.size() < 2)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "KICK");
        return;
    }

    std::vector<std::string> channelNames = _parser.splitByComma(params[0]);
    std::vector<std::string> namesToKick = _parser.splitByComma(params[1]);

    if (channelNames.size() != namesToKick.size())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "KICK");
        return;
    }

    // check channels     # || &      && > 2
    for (size_t i = 0; i < channelNames.size(); i++)
    {
        if (channelNames[i].size() < 2)
        {
            // error
            return;
        }
    }
    // check userNames size > 0
    for (size_t i = 0; i < namesToKick.size(); i++)
    {
        if (namesToKick[i].size() < 1)
        {
            //error
            return;
        }
    }

    // для каждого channel и userNames
    for (size_t i = 0; i < namesToKick.size(); i++)
    {
        Channel *ch;
        if (!_channels.count(channelNames[i]))
        {
            sendError(c, ERR_NOSUCHCHANNEL, channelNames[i]);
            continue;;
        }
        ch = _channels[channelNames[i]];
        if (!ch->isUser(c->getNick()))
        {
            sendError(c, ERR_NOTONCHANNEL, channelNames[i]);
            return;
        }
        if (!ch->getOperators().count(c->getNick()))
        {
            sendError(c, ERR_CHANOPRIVSNEEDED, channelNames[i]);
            return;
        }

        // проверяем ник в канале
        if (!ch->isUser(namesToKick[i]))
        {
            sendError(c, ERR_USERNOTINCHANNEL, namesToKick[i]);
            return;
        }

        Client *userToKick = NULL;
        for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); it++)
        {
            if (it->second->getNick() == namesToKick[i])
            {
                userToKick = it->second;
                break;
            }
        }
        if (!userToKick)
        {
            //error
        }

        // fds to send users ( + deletet user)
        // std::vector<int> fds;
        // std::set<std::string> &chUsers = ch->getUsers();
        // for (std::map<std::string, Client *>::iterator it = chUsers.begin(); it != chUsers.end(); it++)
        // {
        //     fds.push_back(it->second->getFD());
        // }
        //удаляем
        // kickClientFromChannel(*ch, userToKick);
        // сообщаем всем fds
        // std::string outMessage = ":" + c->getNick() + " KICK " + channelNames[i] + " " + userToKick->getNick() + " :" + command.getText() + "\r\n";
        // for (size_t d = 0; d < fds.size(); i++)
        // {
        //     send_message_to_client(c->getFD(), outMessage);
        // }
    }

    //         Examples :
    //         : Angel INVITE Wiz #Dust; User Angel inviting WiZ to channel #Dust
    //         INVITE Wiz #Twilight_Zone; Command to invite WiZ to #Twilight_zone
}

void Server::kickClientFromChannel(Channel &channel, Client *client)
{
    const std::string kickedNick = client->getNick();
    std::set<std::string> &users = channel.getUsers();
    std::set<std::string> &operators = channel.getOperators();
    std::set<std::string> &invited = channel.getInvited();

    users.erase(client->getNick());
    operators.erase(client->getNick());
    invited.erase(client->getNick());
}

void Server::help(Client *c)
{
    std::string pass = "command PASS <password>\n";
    std::string nick = "command NICK <nickname>\n";
    std::string user = "command USER <username> <hostname> <servername> :<realname>\n";
    std::string join = "command JOIN #channel key\n";
    std::string mode = "command MODE <#channel> {[+|-]|o|p|s|i|t|n|b|v} [<limit>] [<user>]\n";
    std::string topic = "command TOPIC <#channel>\n";
    std::string topic2 = "command TOPIC <#channel> :<topic>\n";
    std::string list = "command LIST <#channel> [<topic>]\n";
    std::string invite = "command INVITE <nickname> <#channel>\n";
    std::string kick = "command KICK <#channel> <user> [<comment>]\n";
    std::string privmsg = "command PRIVMSG <receiver>{,<receiver>} :<text to be sent>\n";

    if (!c->getRegStatus())
        c->enqueue_reply(YELLOW + pass + nick + user + RESET "\r\n");
    else
        c->enqueue_reply(YELLOW + join + mode + topic + list + invite + kick + privmsg + RESET "\r\n");

    set_event_for_sending_msg(c->getFD());
    
    //     send_message_to_client(c->getFD(), ":" + pass + nick + user + "\r\n");
    // else
    //     send_message_to_client(c->getFD(), ":" + join + mode + topic + topic2 + list + invite + kick + privmsg + "\r\n");

}

void Server::topic(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    std::vector<std::string> params = _parser.splitParams(command.getMode());
    std::cout << "TOPIC Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.size() < 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "TOPIC");
        return;
    }
    std::string channelName = params[0];

    if (channelName.empty() || channelName.size() < 2 || (channelName[0] != '#' && channelName[0] != '&'))
    {
        sendError(c, ERR_NOSUCHCHANNEL, channelName);//TODO ERROR wrong arg ?
        return;
    }

    //TODO!!! check chanelName

    Channel *ch;
    if (_channels.count(channelName) == 0)
    {
        sendError(c, ERR_NOSUCHCHANNEL, channelName);
        return;
    }
    ch = _channels[channelName];

    if (!ch->isUser(c->getNick()))
    {
        sendError(c, ERR_NOTONCHANNEL, channelName);
        return;
    }

    if (command.getText().empty())
    {
        if (ch->getTopic().empty())
            sendError(c, RPL_NOTOPIC, channelName);
        else
        {
            std::string msg = channelName + " :" + ch->getTopic() + "\r\n"; // 332 RPL_TOPIC "<channel> :<topic>"
            send_message_to_client(c->getFD(), msg);
        }
    }
    else
    {
        if (ch->isT() && !ch->isOperator(c->getNick()))
        {
            sendError(c, ERR_CHANOPRIVSNEEDED, channelName);
            return;
        }
        if (ch->getTopic().empty())
        {
            ch->setTopic(command.getText());
            std::string msg = ":Topic " + command.getText() + " on " + channelName + " seted\r\n";
            send_message_to_client(c->getFD(), msg);
        }
        else
        {
            std::string old = ch->getTopic();
            ch->setTopic(command.getText());
            std::string msg = ":User " + c->getNick() + " changed topic from " + old + " to " + command.getText() + " on " + channelName + "\r\n";
            send_message_to_client(c->getFD(), msg);
        }
    }
}

void Server::cap(Client *c, const Command &command)
{
    std::vector<std::string> params = _parser.splitParams(command.getMode());
    if (params.size() == 0) return;
    if (params[0] == "LS") 
    {
        c->enqueue_reply(":server CAP * LS :\r\n");
        set_event_for_sending_msg(c->getFD());
        // send_message_to_client(c->getFD(), ":server CAP * END\r\n");
    }
    // else if (params[0] == "END") {}
}

void Server::ping(Client *c, const Command &command)
{
    std::string arg = command.getMode();

    if (arg.empty())
        return; // по RFC можно игнорировать

    send_message_to_client(c->getFD(), "PONG :" + arg + "\r\n");
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

    c->enqueue_reply(RED ":server " + s + " " + nick + " " + message + RESET "\r\n");
    set_event_for_sending_msg(c->getFD());
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
    case ERR_USERNOTINCHANNEL:
        return "<nick> <channel> :They aren't on that channel";
    case ERR_CHANOPRIVSNEEDED:
        return "<channel> :You're not channel operator";
    case RPL_NOTOPIC:
        return "<channel> :No topic is set";
    case ERR_NOTONCHANNEL:
        return "<channel> :You're not on that channel";
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
    case ERR_NORECIPIENT:
        return "<nick> :No recipient given (PRIVMSG)";
    case ERR_NOTEXTTOSEND:
        return "<nick> :No text to send";
    case ERR_CANNOTSENDTOCHAN:
        return "<nick> #secret :Cannot send to channel";
    case ERR_NOTOPLEVEL:
        return "<nick> mask :No toplevel domain specified";
    case ERR_TOOMANYTARGETS:
        return "<nick> a,b,c,d,e,f :Too many targets";
    case ERR_NOSUCHNICK:
        return "<nick> UnknownGuy :No such nick/channel";
    case RPL_AWAY:
        return "<your_nick> Alice :I am sleeping";
    default:
        return ":Error";
    }
}

void Server::process_line(Client *c, std::string line)
{
    std::cout << "line: " << line << std::endl;
    _parser.trim(line);
    if (line.empty())
    {
        return;
        Command("", NOT_FOUND, "", ""); //???
    }

    Command cmnd = _parser.parse(*c, line);

    if (cmnd.getCommand() == NOT_FOUND) {
        c->enqueue_reply("Command not found <" + line + ">\r\n");
        set_event_for_sending_msg(c->getFD());
    }

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
    else if (cmnd.getCommand() == PING)
        ping(c, cmnd);
}

void Server::set_event_for_sending_msg(int fd) {
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds[i].events |= POLLOUT;
            break;
        } 
    }
}


void Server::sendWelcome(Client *c)
{
    std::string nick = c->getNick();

    c->enqueue_reply(":server 001 " + nick + " :Welcome to server!!!!!!\r\n");
    c->enqueue_reply(":server 002 " + nick + " :Your host is server\r\n");
    c->enqueue_reply(":server 003 " + nick + " :This server was created today\r\n");

    c->enqueue_reply(":server 375 " + nick + " :server Message\r\n");

    c->enqueue_reply(":server 372 " + nick + " :Welcome!\r\n");

    c->enqueue_reply(":server 376 " + nick + " :END!\r\n");

    set_event_for_sending_msg(c->getFD());
}

void Server::privmsg(Client *c, const Command &cmd) {
    if (cmd.getText().empty()) {sendError(c, ERR_NOTEXTTOSEND, "PRIVMSG"); return; };
    if (cmd.getMode().empty()) {sendError(c, ERR_NORECIPIENT, "PRIVMSG"); return; };

    std::string R = RESET;
    std::stringstream ss(cmd.getMode());
    std::string target;
    while (std::getline(ss, target,',')) {
        if (target[0] == '#') {
            // send message to target in group
            std::map<std::string, Channel*>::iterator it = _channels.find(target);
            if (it == _channels.end()) {sendError(c, ERR_NOSUCHNICK, c->getNick()); return; };
            std::string sender = (c->getNick().empty() ? "*" : c->getNick());
            it->second->broadcast(c, ":" GREEN + sender + R + " PRIVMSG " + BLUE + target + R + " :" MAGENTA + cmd.getText() + R + "\r\n");
        } else {
            // send message to target
            std::map<std::string, Client*>::iterator it = _nicks.find(target);
            if (it == _nicks.end()) {sendError(c, ERR_NOSUCHNICK, c->getNick()); return; };
            std::string sender = (c->getNick().empty() ? "*" : c->getNick());
            it->second->enqueue_reply(":" GREEN + sender + R + " PRIVMSG " + BLUE + target + R + " :" MAGENTA + cmd.getText() + R + "\r\n");
            set_event_for_sending_msg(it->second->getFD());
        }
    }
}