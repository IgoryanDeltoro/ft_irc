#include "../../includes/Server.hpp"



        //    ERR_CANNOTSENDTOCHAN            ERR_NOTOPLEVEL
        //    ERR_WILDTOPLEVEL                
        //    RPL_AWAY

void Server::privmsg(Client *c, const Command &cmd) {
    if (!isClientAuth(c)) return;
    if (cmd.getParams().empty()) { sendNumericReply(c, ERR_NORECIPIENT, "PRIVMSG", ""); return; }
    if (cmd.getText().empty()) { sendNumericReply(c, ERR_NOTEXTTOSEND, "PRIVMSG", ""); return; }
    const std::string arg1 = cmd.getParams()[0];
    const std::vector<std::string> ch_mem = _parser.splitByComma(arg1);
    // if (ch_mem.size() > 4) {
    //     sendNumericReply(c, ERR_TOOMANYTARGETS, "PRIVMSG", "");
    //     return;
    // }
    for (size_t i = 0; i < ch_mem.size(); ++i) {
        const std::string lower = _parser.ircLowerStr(ch_mem[i]);
        if (lower[0] == '#' || lower[0] == '&') {
            std::map<std::string, Channel*>::iterator channel = _channels.find(lower);
            if (channel == _channels.end()) {
                sendNumericReply(c, ERR_NOSUCHCHANNEL, "", ch_mem[i]);
                continue;
            }
            if (!channel->second->isUser(c->getNickLower())) {
                sendNumericReply(c, ERR_CANNOTSENDTOCHAN, "", ch_mem[i]);
                continue;
            }
            channel->second->broadcast(c, c->buildPrefix() + " PRIVMSG " + ch_mem[i] + " :" + cmd.getText() + "\r\n");
            set_event_for_group_members(channel->second, true);
        }
        else {
            std::map<std::string, Client*>::iterator it = _nicks.find(lower);
            if (it == _nicks.end()) {
                sendNumericReply(c, ERR_NOSUCHNICK, ch_mem[i], "");
                continue;
            }
            it->second->enqueue_reply(c->buildPrefix() + " PRIVMSG " + ch_mem[i] + " :" + cmd.getText() + "\r\n");
            set_event_for_sending_msg(it->second->getFD(), true);
        }
    }
}