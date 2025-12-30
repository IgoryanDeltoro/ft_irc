#include "../../includes/Server.hpp"

//       Command: JOIN
//   Parameters: ( <channel> *( "," <channel> ) [ <key> *( "," <key> ) ] ) / "0"
void Server::join(Client *c, const Command &command)
{
    const std::vector<std::string> &params = command.getParams();
    if (params.empty()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "JOIN", "");
        return;
    }
    const std::string &channelsRaw = params[0];

    if (channelsRaw == "0") {
        const std::set<std::string> channels = c->getChannels();
        for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it) {
            std::cout << "00000 \n";
            
            if (_channels.count(*it) == 0) continue;
            
            std::cout << "0000 11111 \n";
            Channel *ch = _channels[*it];
            std::cout << "--statr PART \n";
            partFromChannel(c, ch, c->getNick());
            std::cout << "--end PART \n";
        }
        return;
    }

    const std::vector<std::string> channelNames = _parser.splitByComma(channelsRaw);
    std::vector<std::string> keys;

    if (params.size() > 1) {
        const std::string &keysRaw = params[1];
        keys = _parser.splitByComma(keysRaw);
    }

    for (size_t i = 0; i < channelNames.size(); i++) {
        const std::string &channelName = channelNames[i];
        const std::string key = (i < keys.size() ? keys[i] : "");
        if (!_parser.isValidChannelName(channelName)) {
            sendNumericReply(c, ERR_BADCHANMASK, "", channelName);
            continue;
        }
        if (c->getChannelSize() >= 10) {
            sendNumericReply(c, ERR_TOOMANYCHANNELS, "", channelName);
            continue;
        }
        joinChannel(c, channelName, key);
    }
}

void Server::joinChannel(Client *c, const std::string &name, const std::string &password)
{
    const std::string lower = _parser.ircLowerStr(name);
    Channel *ch = NULL;
    if (_channels.count(lower) == 0) {
        ch = new Channel(name, lower, c);
        if (!password.empty()) {
            ch->setK(true, password);
        }
        _channels[lower] = ch;
    }
    else {
        ch = _channels[lower];
        if (ch->isI() && !ch->isInvited(c->getNickLower())) {
            sendNumericReply(c, ERR_INVITEONLYCHAN, "", name);
            return;
        }
        if (ch->isK() && ch->getPassword() != password) {
            sendNumericReply(c, ERR_BADCHANNELKEY, "", name);
            return;
        }
        if (ch->isL() && ch->getUserLimit() > 0 && (int)ch->getUsers().size() >= ch->getUserLimit()) {
            sendNumericReply(c, ERR_CHANNELISFULL, "", name);
            return;
        }
        if (ch->isUser(c->getNickLower()))
            return;
        ch->addUser(c);
        c->addToChannel(lower);
        ch->removeInvite(c->getNickLower());
        //todo remove from invites???
    }
    const std::string joinMsg = ":" + c->buildPrefix() + " JOIN " + name + "\r\n";
    c->enqueue_reply(joinMsg);
    set_event_for_sending_msg(c->getFD(), true);
    if (ch->getTopic().empty()) sendNumericReply(c, RPL_NOTOPIC, "", name);
    else sendNumericReply(c, RPL_TOPIC, ch->getTopic(), name);
    sendNumericReply(c, RPL_NAMREPLY, ch->getNamesList(), name);
    sendNumericReply(c, RPL_ENDOFNAMES, "", name);
    ch->broadcast(c, joinMsg);
    set_event_for_group_members(ch, true);
}
