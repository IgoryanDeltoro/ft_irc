#include "../../includes/Server.hpp"

void Server::privmsg(Client *c, const Command &cmd) {
    if (!isClientAuth(c)) return;
    if (cmd.getParams().empty()) { sendNumericReply(c, ERR_NORECIPIENT, "PRIVMSG", ""); return; }
    if (cmd.getText().empty()) { sendNumericReply(c, ERR_NOTEXTTOSEND, "PRIVMSG", ""); return; }
    const std::string arg1 = cmd.getParams()[0];
    const std::vector<std::string> ch_mem = _parser.splitByComma(arg1);
    if (ch_mem.size() > 4) {
        sendNumericReply(c, ERR_TOOMANYTARGETS, "PRIVMSG", "");
        return;
    }
    for (size_t i = 0; i < ch_mem.size(); ++i) {
        std::string lower = _parser.ircLowerStr(ch_mem[i]);
        if (lower[0] == '#' || lower[0] == '&') {
            // send message to targets into the group
            std::map<std::string, Channel*>::iterator channel = _channels.find(lower);
            if (channel == _channels.end()) {
                sendNumericReply(c, ERR_NOSUCHNICK, "", ch_mem[i]);
                return;
            };

            channel->second->broadcast(c, c->buildPrefix() + " PRIVMSG " + ch_mem[i] + " :" + cmd.getText() + "\r\n");
            set_event_for_group_members(channel->second, true);
        } else {
            // send message to target
            std::map<std::string, Client*>::iterator it = _nicks.find(lower);
            if (it == _nicks.end()) { sendNumericReply(c, ERR_NOSUCHNICK, ch_mem[i], ""); return; };

            it->second->enqueue_reply(c->buildPrefix() + " PRIVMSG " + ch_mem[i] + " :" + cmd.getText() + "\r\n");
            set_event_for_sending_msg(it->second->getFD(), true);
        }
    }
}