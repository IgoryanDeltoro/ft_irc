#include "../../includes/Server.hpp"

void Server::nick(Client *c, const Command &command)
{
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
    const std::string &oldNick = c->getNick();
    const std::string &oldNickLower = c->getNickLower();
    
    
    std::cout << "-nick-: oldNick: " << oldNick << ", oldNickLower: " << oldNickLower << ", newNick: " << newNick << ", newNickLower:" << newNickLower << std::endl;



    if (oldNick == newNick) {
        std::cout << "-nick-: oldNick == newNick\n";
        return;
    } 
    
    if (oldNickLower == newNickLower) {
        c->setNick(newNick);
        std::cout << "-nick-: oldNickLower == newNickLower\n";
    }
    else {
        std::cout << "-nick-: oldNickLower != newNickLower\n";
        if (isNickExists(newNickLower)) {
            sendNumericReply(c, ERR_NICKNAMEINUSE, newNick, "");
            return;
        }
        c->setNick(newNick);
        c->setNickLower(newNickLower);
        _nicks[newNickLower] = c;
    }

    if (!c->getRegStatus() && !c->getUserName().empty() && !c->getRealName().empty()) {
        c->setRegStatus(true);
        sendWelcome(c);
        std::cout << "-nick-: registration!\n";
        return;
    }

    const std::string msg = ":" + oldNick + "!" + c->getUserName() + "@" + c->getHost() + " NICK " + newNick + "\r\n";
    c->enqueue_reply(msg);
    set_event_for_sending_msg(c->getFD(), true);
    if (!c->getRegStatus()) return;
    std::set<Client *> notify;
    const std::set<std::string> &clientChannels = c->getChannels();

    for (std::set<std::string>::const_iterator it = clientChannels.begin(); it != clientChannels.end(); ++it) {
        const std::string &chanLower = *it;
        std::map<std::string, Channel *>::iterator chIt = _channels.find(chanLower);
        if (chIt == _channels.end()) continue;
        Channel *ch = chIt->second;
        if (ch->isUser(oldNickLower)) {
            ch->removeUser(oldNickLower);
            ch->addUser(c);
            if (ch->isOperator(oldNickLower)) {
                ch->removeOperator(oldNickLower);
                ch->addOperator(newNickLower);
            }
            if (ch->isInvited(oldNickLower)) {
                ch->removeInvite(oldNickLower);
                ch->addInvite(newNickLower);
            }
            const std::map<std::string, Client *> &users = ch->getUsers();
            for (std::map<std::string, Client *>::const_iterator u = users.begin(); u != users.end(); ++u)
                notify.insert(u->second);
        }
    }
    for (std::set<Client *>::const_iterator it = notify.begin(); it != notify.end(); ++it) {
        Client *other = *it;
        if (other != c) {
            other->enqueue_reply(msg);
            set_event_for_sending_msg(other->getFD(), true);
        }
    }

    std::cout << "NICKS in _nicks before: ";
    for(std::map<std::string, Client *>::iterator it = _nicks.begin(); it != _nicks.end(); ++it)
    {
        std::cout << it->first << " | ";
    }
    std::cout << std::endl;


    if (!oldNick.empty() && oldNickLower != newNickLower) {
        std::map<std::string, Client *>::iterator it = _nicks.find(oldNickLower);
        if (it == _nicks.end()) {
             std::cout << "-nick-: OLD not FOUND!!!!!!!" <<"\n";
            return;
        } 
        std::cout << "-nick-: OLD FOUND!" << it->second->getNick() <<"\n";
        _nicks.erase(it);
    }

    std::cout << "NICKS in _nicks after: ";
    for(std::map<std::string, Client *>::iterator it = _nicks.begin(); it != _nicks.end(); ++it)
    {
        std::cout << it->first << " | ";
    }
    std::cout << std::endl;
}
