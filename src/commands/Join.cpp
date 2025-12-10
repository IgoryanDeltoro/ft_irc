#include "../../includes/Server.hpp"

void Server::join(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;

    std::vector<std::string> params = command.getParams();
    if (params.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "JOIN", "");
        return;
    }

    std::string channelsRaw = params[0];
    _parser.trim(channelsRaw);
    std::vector<std::string> channelNames = _parser.splitByComma(channelsRaw);
    std::vector<std::string> keys;

    if (params.size() > 1)
    {
        std::string keysRaw = params[1];
        _parser.trim(keysRaw);
        keys = _parser.splitByComma(keysRaw);
    }

    for (size_t i = 0; i < channelNames.size(); i++)
    {
        std::string channelName = channelNames[i];
        std::string key = (i < keys.size() ? keys[i] : "");
        if (!_parser.isValidChannelName(channelName))
        {
            sendError(c, ERR_NOSUCHCHANNEL, "", channelName);
            continue;
        }
        if (c->getChannelSize() >= 10)
        {
            sendError(c, ERR_TOOMANYCHANNELS, "", channelName);
            continue;
        }
        joinChannel(c, channelName, key);
    }
}

void Server::joinChannel(Client *c, const std::string &name, const std::string &password)
{
    std::string lower = _parser.ircLowerStr(name);
    Channel *ch = NULL;
    if (_channels.count(lower) == 0) {
        ch = new Channel(name, lower, c);
        if (!password.empty()) {
            ch->setK(true, password);
        }
        _channels[lower] = ch;
    }
    else
    {
        ch = _channels[lower];

        if (ch->isI() && !ch->isInvited(c->getNickLower())) {
            sendError(c, ERR_INVITEONLYCHAN, "", name);
            return;
        }
        if (ch->isK() && ch->getPassword() != password) {
            sendError(c, ERR_BADCHANNELKEY, "", name);
            return;
        }
        if (ch->isL() && ch->getUserLimit() > 0 && (int)ch->getUsers().size() >= ch->getUserLimit())
        {
            sendError(c, ERR_CHANNELISFULL, "", name);
            return;
        }
        if (ch->isUser(c->getNickLower()))
            return;
        ch->addUser(c);
    }
    
    if (ch->getTopic().empty())
        c->enqueue_reply(":server 331 " + c->getNick() + " " + name + " :No topic is set\r\n");
    else
        c->enqueue_reply(":server 332 " + c->getNick() + " " + name + " :" + ch->getTopic() + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);

    std::string namesList = getNamesList(c, ch);
    c->enqueue_reply(namesList);
    c->enqueue_reply(":server 366 " + c->getNick() + " " + name + " :End of /NAMES list.\r\n");
    set_event_for_sending_msg(c->getFD(), true);

    std::string joinMsg = c->buildPrefix() + " JOIN " + name + "\r\n";
    ch->broadcast(c, joinMsg);
    set_event_for_group_members(ch, true);
}

std::string Server::getNamesList(Client *c, Channel *ch)
{
    std::string nick = c->getNick();
    std::string namesList = ":server 353 " + nick + " = " + ch->getName() + " :";
    const std::map<std::string, Client *> &users = ch->getUsers();
    for (std::map<std::string, Client *>::const_iterator it = users.begin(); it != users.end(); ++it)
    {
        Client *user = it->second;
        std::string prefix;
        if (ch->isOperator(user->getNickLower()))
            prefix = "@";
        namesList += prefix + user->getNick() + " ";
    }
    namesList += "\r\n";
    return namesList;
}
