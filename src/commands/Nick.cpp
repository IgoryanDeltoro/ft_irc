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
    const std::string newNickLower = _parser.ircLowerStr(newNick);
    if (!_parser.isValidNick(newNick)) {
        sendNumericReply(c, ERR_ERRONEUSNICKNAME, newNick, "");
        return;
    }
    const std::string &oldNick = c->getNick();
    const std::string &oldNickLower = c->getNickLower();
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
        _nicks[newNickLower] = c;
    }
    if (oldNick.empty()) {
        if (!c->getUserName().empty() && !c->getRealName().empty() && !c->getRegStatus()) {
            c->setRegStatus(true);
            sendWelcome(c);
        }
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
    if (!oldNick.empty() && oldNickLower != newNickLower)  _nicks.erase(oldNickLower);
}
