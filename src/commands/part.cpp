#include "../../includes/Server.hpp"

void Server::part(Client *c, const Command &command)
{
    const std::vector<std::string> &params = command.getParams();
    
    if (params.size() < 1) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "PART", "");
        return;
    }

    const std::string &channelNamesRaw = params[0];
    const std::vector<std::string> channelNames = _parser.splitByComma(channelNamesRaw);
    const std::string partMsg = command.getText().empty() ? c->getNick(): command.getText();

    for (size_t i = 0; i < channelNames.size(); i++) {
        const std::string channelNameLower = _parser.ircLowerStr(channelNames[i]);
        if (_channels.count(channelNameLower) == 0) {
            sendNumericReply(c, ERR_NOSUCHCHANNEL, "", channelNames[i]);
            continue;
        }
        Channel *ch = _channels[channelNameLower];
        partFromChannel(c, ch, partMsg);
    }
}

void Server::partFromChannel(Client *c, Channel *ch, const std::string &msg)
{
        if (!ch->isUser(c->getNickLower())) {
            sendNumericReply(c, ERR_NOTONCHANNEL, "", ch->getName());
            return;
        }

        const std::string outMessage = ":" + c->buildPrefix() + " PART " + ch->getName() + " :" + msg + "\r\n";
    
        ch->broadcast(NULL, outMessage);
        set_event_for_group_members(ch, true);

        c->removeChannel(ch->getNameLower());
        ch->removeInvite(c->getNickLower());
        ch->removeOperator(c->getNickLower());
        ch->removeUser(c->getNickLower());

        if (ch->getUsers().empty()) {
            _channels.erase(ch->getNameLower());
            delete ch;
            return;
        }
        if (ch->getOperators().size() == 0) {
            //todo new operator + broadcast? or no?
        }
}