#include "../../includes/Server.hpp"

void Server::privmsg(Client *c, const Command &cmd) {
    if (!isClientAuth(c))
        return;
    if (cmd.getParams().empty()) {
        sendNumericReply(c, ERR_NORECIPIENT, "PRIVMSG", "");
        return;
    }
    if (cmd.getText().empty()) {
        sendNumericReply(c, ERR_NOTEXTTOSEND, "PRIVMSG", "");
        return;
    }

    const std::vector<std::string> targets = _parser.splitByComma(cmd.getParams()[0]);
    std::set<std::string> uniques;

    for (size_t i = 0; i < targets.size(); ++i) {
        const std::string &target = targets[i];
        if (target.empty()) continue;

        const std::string lower = _parser.ircLowerStr(target);
        if (uniques.find(lower) != uniques.end()) continue;
        uniques.insert(lower);

        if (lower[0] == '#' || lower[0] == '&') {
            if (!_parser.isValidChannelName(target)) {
                sendNumericReply(c, ERR_NOSUCHCHANNEL, "", target);
                continue;
            }
            std::map<std::string, Channel*>::iterator channel = _channels.find(lower);
            if (channel == _channels.end()) {
                sendNumericReply(c, ERR_NOSUCHCHANNEL, "", target);
                continue;
            }
            Channel *ch = channel->second;
            if (!ch) {
                sendNumericReply(c, ERR_NOSUCHCHANNEL, "", target);
                continue;
            }
            if (!ch->isUser(c->getNickLower())) {
                sendNumericReply(c, ERR_CANNOTSENDTOCHAN, "", target);
                continue;
            }
            ch->broadcast(c, c->buildPrefix() + " PRIVMSG " + target + " :" + cmd.getText() + "\r\n");
            set_event_for_group_members(ch, true);
        }
        else {
            if (!_parser.isValidNick(target)) {
                sendNumericReply(c, ERR_NOSUCHNICK, target, "");
                continue;
            }
            std::map<std::string, Client*>::iterator it = _nicks.find(lower);
            if (it == _nicks.end()) {
                sendNumericReply(c, ERR_NOSUCHNICK, target, "");
                continue;
            }
            Client *cl = it->second;
            if (!cl || !cl->getRegStatus()) {
                sendNumericReply(c, ERR_NOSUCHNICK, target, "");
                continue;
            }
            cl->enqueue_reply(c->buildPrefix() + " PRIVMSG " + target + " :" + cmd.getText() + "\r\n");
            set_event_for_sending_msg(cl->getFD(), true);

            if (cl->isAway()) {
                sendNumericReply(c, RPL_AWAY, cl->getNick(), cl->getAwayMsg());
            }
        }
    }
}
