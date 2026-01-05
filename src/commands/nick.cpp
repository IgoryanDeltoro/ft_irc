#include "../../includes/Server.hpp"

void Server::nick(Client *c, const Command &command) {
    if (!c->getPassStatus()) {
        std::cout << MAGENTA << c->buildPrefix() << RED " password not set!\n" RESET;
        return;
    }

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
    const std::string currentNick = c->getNick();
    const std::string currentNickLower = c->getNickLower();

    if (currentNick.empty()) {
        if (isNickExists(newNickLower)) {
            sendNumericReply(c, ERR_NICKNAMEINUSE, newNick, "");
            return;
        }
        c->setNick(newNick);
        c->setNickLower(newNickLower);
        _nicks[newNickLower] = c;
        
        std::cout << MAGENTA << c->buildPrefix() << RESET " set nick: " << newNick << " (" << newNickLower << ")" << std::endl;

        if (!c->getUserName().empty() && !c->getRealName().empty()) {
            c->setRegStatus(true);
            sendWelcome(c);
        }
        return;
    }

    if (currentNick == newNick) return;

    if (currentNickLower == newNickLower) {
        c->setNick(newNick);
        std::cout << MAGENTA << c->buildPrefix() << RESET " set nick: " GREEN << newNick << RESET "" << std::endl;
    } else {
        if (isNickExists(newNickLower)) {
            sendNumericReply(c, ERR_NICKNAMEINUSE, newNick, "");
            return;
        }
        c->setNick(newNick);
        c->setNickLower(newNickLower);
        c->addNickHistory(currentNickLower);
        _nicks[newNickLower] = c;

        std::cout << MAGENTA << c->buildPrefix() << RESET " set nick: " << newNick << " (" << newNickLower << ")" << std::endl;
    }

    const std::string msg = ":" + currentNick + "!" + c->getUserName() + "@" + c->getHost() + " NICK " + newNick + "\r\n";
    c->enqueue_reply(msg);
    set_event_for_sending_msg(c->getFD(), true);

    if (!c->getRegStatus()) return;

    std::set<Client *> notify;
    const std::set<std::string> &clientChannels = c->getChannels();

    
    std::set<std::string>::const_iterator it = clientChannels.begin();
    for (; it != clientChannels.end(); ++it) {
        const std::string &chanLower = *it;

        std::map<std::string, Channel *>::iterator chIt = _channels.find(chanLower);
        if (chIt == _channels.end()) {
            continue;
        }
        Channel *ch = chIt->second;

        if (!ch->isUser(currentNickLower)) continue;

        if (currentNickLower != newNickLower) {
            ch->addUser(c);
            ch->removeUser(currentNickLower);

            if (ch->isOperator(currentNickLower)) {
                ch->removeOperator(currentNickLower);
                ch->addOperator(newNickLower);
            }
            if (ch->isInvited(currentNickLower)) {
                ch->removeInvite(currentNickLower);
                ch->addInvite(newNickLower);
            }
        }
        const std::map<std::string, Client *> &users = ch->getUsers();
        for (std::map<std::string, Client *>::const_iterator u = users.begin(); u != users.end(); ++u)
            notify.insert(u->second);
    }

    for (std::set<Client *>::const_iterator it = notify.begin(); it != notify.end(); ++it) {
        Client *other = *it;
        if (other != c) {
            other->enqueue_reply(msg);
            set_event_for_sending_msg(other->getFD(), true);
        }
    }

    if (currentNickLower != newNickLower) {
        std::map<std::string, Client *>::iterator it = _nicks.find(currentNickLower);
        if (it == _nicks.end()) {
            return;
        }
        _nicks.erase(it);
    }
}
