#include "../../includes/Server.hpp"

void Server::mode(Client *c, const Command &command)
{
    if (!isClientAuth(c)) return;
    const std::vector<std::string> params = command.getParams();
    if (params.empty()) {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "MODE", "");
        return;
    }
    const std::string channelName = params[0];
    const std::string channelNameLower = _parser.ircLowerStr(channelName);
    if (_channels.count(channelNameLower) == 0) {
        sendNumericReply(c, ERR_NOSUCHCHANNEL, "", channelName);
        return;
    }
    Channel *ch = _channels[channelNameLower];
    if (!ch->isUser(c->getNickLower())) {
        sendNumericReply(c, ERR_NOTONCHANNEL, "", channelName);
        return;
    }
    if (params.size() == 1) {
        sendNumericReply(c, RPL_CHANNELMODEIS, ch->getAllModesString(), ch->getName());
        return;
    }
    if (!ch->isOperator(c->getNickLower())) {
        sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", channelName);
        return;
    }
    const std::string modeStr = params[1];
    std::vector<std::string> args;
    for (size_t i = 2; i < params.size(); ++i) args.push_back(params[i]);
    std::string addModeStr;
    std::string removeModeStr;
    std::vector<std::string> addModeArgs;
    std::vector<std::string> removeModeArgs;
    bool adding = true;
    int oLimit = 0;
    size_t argIndex = 0;
    for (size_t i = 0; i < modeStr.size(); i++) {
        const char f = modeStr[i];
        if (f == '+') { adding = true; continue; }
        if (f == '-') { adding = false; continue; }
        applyChannelMode(c, ch, f, adding, args, argIndex, addModeStr, removeModeStr, addModeArgs, removeModeArgs, oLimit);
    }
    if (addModeStr.empty() && removeModeStr.empty()) return;
    if (!addModeStr.empty()) addModeStr = "+" + addModeStr;
    if (!removeModeStr.empty()) removeModeStr = "-" + removeModeStr;
    std::string modeMsg = ":" + c->buildPrefix() + " MODE " + ch->getName() + " " + addModeStr + removeModeStr;
    for (size_t i = 0; i < addModeArgs.size(); i++) modeMsg += " " + addModeArgs[i];
    for (size_t i = 0; i < removeModeArgs.size(); i++) modeMsg += " " + removeModeArgs[i];
    modeMsg += "\r\n";

    std::cout << YELLOW "sending mode changing:" RESET << modeMsg;

    ch->broadcast(NULL, modeMsg);
    set_event_for_group_members(ch, true);
}

void Server::applyChannelMode(Client *c, Channel *channel, char f, bool adding, std::vector<std::string> &args, size_t &argIndex, std::string &addModeStr, std::string &removeModeStr, std::vector<std::string> &addModeArgs, std::vector<std::string> &removeModeArgs, int &oLimit)
{
    switch (f) {
    case 'i': {
        
        if (adding) {
            if(channel->isI()) return;
            addModeStr += 'i';
        }
        else {
            if (!channel->isI())  return;
            removeModeStr += 'i';
        }
        channel->setI(adding);
        return;
    }
    case 't': {
        if (adding) {
            if (channel->isT()) return;
            addModeStr += 't';
        }
        else {
            if (!channel->isT()) return;
            removeModeStr += 't'; 
        }
        channel->setT(adding);
        return;
    }
    case 'k': {
        if (adding) {
            if (channel->isK()) {
                sendNumericReply(c, ERR_KEYSET, "", channel->getName());
                return;
            }
            if (argIndex >= args.size()) {
                sendNumericReply(c, ERR_NEEDMOREPARAMS, "MODE", "");
                return;
            }
            std::string pass = args[argIndex++];
            channel->setK(true, pass);
            addModeStr += 'k';
            addModeArgs.push_back(pass);
        }
        else {
            if (channel->isK()) {
                channel->setK(false, "");
                removeModeStr += 'k';
            }
        }
        return;
    }
    case 'l':
    {
        if (adding) {
            if (argIndex >= args.size()) {
                sendNumericReply(c, ERR_NEEDMOREPARAMS, "MODE", "");
                return;
            }
            int lim;
            std::string limStr = args[argIndex++];
            std::istringstream iss(limStr);
            if (!(iss >> lim)) return;
            if (lim <= 0) return;
            channel->setL(lim);
            addModeStr += 'l';
            addModeArgs.push_back(limStr);
        }
        else {
            if (!channel->isL()) return;
            channel->setL(-1);
            removeModeStr += 'l';
        }
        return;
    }
    case 'o':
    {
        if (oLimit >= 3) {
            if (argIndex < args.size())
                argIndex++;
            return;
        }
        oLimit ++;
        if (argIndex >= args.size()) {
            sendNumericReply(c, ERR_NEEDMOREPARAMS, "MODE", "");
            return;
        }
        std::string nick = args[argIndex++];
        std::string nickLower = _parser.ircLowerStr(nick);
        Client *user = getClientByNick(nickLower);
        if (!user) {
            sendNumericReply(c, ERR_NOSUCHNICK, nick, "");
            return;
        }
        if (!channel->isUser(nickLower)) {
            sendNumericReply(c, ERR_USERNOTINCHANNEL, nick, channel->getName());
            return;
        }
        if (adding) {
            if (nickLower == c->getNickLower()) return;
            if (channel->isOperator(nickLower)) return;
            channel->addOperator(user->getNickLower());
            addModeStr += 'o';
            addModeArgs.push_back(nick);
        }
        else {
            if (!channel->isOperator(nickLower)) return;
            channel->removeOperator(user->getNickLower());
            removeModeStr += 'o';
            removeModeArgs.push_back(nick);
        }
        return;
    }
    default: { sendNumericReply(c, ERR_UNKNOWNMODE, std::string(1, f), ""); }
    }
        return;
}
