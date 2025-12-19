#include "../../includes/Server.hpp"

void Server::nick(Client *c, const Command &command)
{
    // DEBUG
    std::cout << "_nicks before: ";
    for(std::map<std::string, Client *>::iterator it = _nicks.begin(); it != _nicks.end(); ++it)
    {
        std::cout << "["  << it->first << "] ";
    }
    std::cout << std::endl;
    // END DEBUG

    if (!c->getPassStatus()) return;
    const std::vector<std::string> &params = command.getParams();
    if (params.size() < 1) {
        sendNumericReply(c, ERR_NONICKNAMEGIVEN, "", "");
        return;
    }

    const std::string &newNick = params[0];
    if (!_parser.isValidNick(newNick)) {
        sendNumericReply(c, ERR_ERRONEUSNICKNAME, newNick, "");
        return;
    }

    const std::string newNickLower = _parser.ircLowerStr(newNick);
    const std::string oldNick = c->getNick();
    const std::string oldNickLower = c->getNickLower();
    
    if (oldNick.empty()) {
        if (isNickExists(newNickLower)) {
            sendNumericReply(c, ERR_NICKNAMEINUSE, newNick, "");
            return;
        }
        c->setNick(newNick);
        c->setNickLower(newNickLower);
        _nicks[newNickLower] = c;
        if (!c->getUserName().empty() && !c->getRealName().empty()) {
            c->setRegStatus(true);
            sendWelcome(c);
        }
        return;
    }

    if (oldNick == newNick) return;
    
    if (oldNickLower == newNickLower) {
        c->setNick(newNick);
    }
    else {
        if (isNickExists(newNickLower)) {
            sendNumericReply(c, ERR_NICKNAMEINUSE, newNick, "");
            return;
        }
        c->setNick(newNick);
        c->setNickLower(newNickLower);
        c->setOldNickLower(oldNickLower);
        _nicks[newNickLower] = c;
    }
    
    const std::string msg = ":" + oldNick + "!" + c->getUserName() + "@" + c->getHost() + " NICK " + newNick + "\r\n";
    c->enqueue_reply(msg);
    set_event_for_sending_msg(c->getFD(), true);

    if (!c->getRegStatus()) return;

    std::set<Client *> notify;
    const std::set<std::string> &clientChannels = c->getChannels();

    
    std::set<std::string>::const_iterator it = clientChannels.begin();
    for (; it != clientChannels.end(); ++it) {
        std::cout << "user has channel: " << *it << std::endl;

        const std::string &chanLower = *it;

        std::map<std::string, Channel *>::iterator chIt = _channels.find(chanLower);
        if (chIt == _channels.end()) {
            std::cout << "Channel not found" << std::endl;
            continue;
        }
        Channel *ch = chIt->second;

        // DEBUG
        std::map<std::string, Client *>::iterator it = ch->getUsers().begin();
        std::cout << "users in channel: ";
        for(; it != ch->getUsers().end(); ++it)
        {
            std::cout << "[" << it->first << "] | ";
        }
        std::cout << std::endl;
        // END DEBUG

        if (!ch->isUser(oldNickLower)) continue;

        if (oldNickLower != newNickLower) {
            ch->addUser(c);
            ch->removeUser(oldNickLower);

            if (ch->isOperator(oldNickLower)) {
                ch->removeOperator(oldNickLower);
                ch->addOperator(newNickLower);
            }
            if (ch->isInvited(oldNickLower)) {
                ch->removeInvite(oldNickLower);
                ch->addInvite(newNickLower);
            }
        }
        const std::map<std::string, Client *> &users = ch->getUsers();
        for (std::map<std::string, Client *>::const_iterator u = users.begin(); u != users.end(); ++u)
            notify.insert(u->second);
    }
    
    // DEBUG 
    std::cout << "Notify users: ";
    for (std::set<Client *>::const_iterator it = notify.begin(); it != notify.end(); ++it) {
        Client *other = *it;
        if (other != c) {
            std::cout << "[" << other->getNick() << "] ";
        }
    }
    std::cout << std::endl;
    // END DEBUG

    for (std::set<Client *>::const_iterator it = notify.begin(); it != notify.end(); ++it) {
        Client *other = *it;
        if (other != c) {
            other->enqueue_reply(msg);
            set_event_for_sending_msg(other->getFD(), true);
        }
    }

    if (oldNickLower != newNickLower) {
        std::map<std::string, Client *>::iterator it = _nicks.find(oldNickLower);
        if (it == _nicks.end()) {
            return;
        } 
        _nicks.erase(it);
    }

    // DEBUG 
    std::cout << "_nicks after: ";
    for(std::map<std::string, Client *>::iterator it = _nicks.begin(); it != _nicks.end(); ++it)
    {
        std::cout << "["  << it->first << "] ";
    }
    std::cout << std::endl;
    // END DEBUG
}