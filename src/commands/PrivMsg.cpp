#include "../../includes/Server.hpp"

void Server::privmsg(Client *c, const Command &cmd) {
    if (!isClientAuth(c)) return;
    if (cmd.getParams().empty()) { sendNumericReply(c, ERR_NORECIPIENT, "PRIVMSG", ""); return; }
    if (cmd.getText().empty()) { sendNumericReply(c, ERR_NOTEXTTOSEND, "PRIVMSG", ""); return; }
    
    const std::vector<std::string> targets = _parser.splitByComma(cmd.getParams()[0]);
    std::set<std::string> uniques;

    for (size_t i = 0; i < targets.size(); ++i) {
        const std::string lower = _parser.ircLowerStr(targets[i]);
        if (lower.empty()) continue;
        if (uniques.find(lower) != uniques.end()) continue;
        uniques.insert(lower);
        if (lower[0] == '#' || lower[0] == '&') {
            std::map<std::string, Channel*>::iterator channel = _channels.find(lower);
            if (channel == _channels.end()) {
                sendNumericReply(c, ERR_NOSUCHCHANNEL, "", targets[i]);
                continue;
            }
            if (!channel->second->isUser(c->getNickLower())) {
                sendNumericReply(c, ERR_CANNOTSENDTOCHAN, "", targets[i]);
                continue;
            }
            channel->second->broadcast(c, c->buildPrefix() + " PRIVMSG " + targets[i] + " :" + cmd.getText() + "\r\n");
            set_event_for_group_members(channel->second, true);
        }
        else {
            std::map<std::string, Client*>::iterator it = _nicks.find(lower);
            if (it == _nicks.end() || !it->second->getRegStatus()) {
                sendNumericReply(c, ERR_NOSUCHNICK, targets[i], "");
                continue;
            }
            it->second->enqueue_reply(c->buildPrefix() + " PRIVMSG " + targets[i] + " :" + cmd.getText() + "\r\n");
            set_event_for_sending_msg(it->second->getFD(), true);
        }
    }
}