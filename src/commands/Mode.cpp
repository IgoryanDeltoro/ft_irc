#include "../../includes/Server.hpp"

void Server::mode(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    // Parameters:
    //  <channel> {[+| -] | o | p | s | i | t | n | b | v}[<limit>][<user>]
    std::vector<std::string> params = command.getParams();
    if (params.empty()) {
        sendError(c, ERR_NEEDMOREPARAMS, "MODE", "");
        return;
    }
    std::string channelName = params[0];
    _parser.trim(channelName);
    if (!channelName.empty() && !(channelName[0] == '#' || channelName[0] == '&'))
        return;

    if (!_parser.isValidChannelName(channelName))
    {
        sendError(c, ERR_NOSUCHCHANNEL, "", channelName);
        return;
    }
    std::string channelNameLower = _parser.ircLowerStr(channelName);
    if (_channels.count(channelNameLower) == 0)
    {
        sendError(c, ERR_NOSUCHCHANNEL, "", channelName);
        return;
    }
    Channel *ch = _channels[channelNameLower];
    if (!ch->isUser(c->getNickLower())) {
        sendError(c, ERR_NOTONCHANNEL, "", channelName);
        return;
    }

    if (!ch->isOperator(c->getNickLower()))
    {
        sendError(c, ERR_CHANOPRIVSNEEDED, "", channelName);
        return;
    }

    if (params.size() == 1)
    {
        std::string modes = ch->getAllModesString(); // собрать строку "+itk ..." с аргументами
        c->enqueue_reply(":server 324 " + c->getNick() + " " + ch->getName() + " " + modes + "\r\n");

        if (!ch->getTopic().empty())
            c->enqueue_reply(":server 332 " + c->getNick() + " " + ch->getName() + " :" + ch->getTopic() + "\r\n");
        
        set_event_for_sending_msg(c->getFD(), true);
        return;
    }

    std::string modeStr = params[1];
    std::vector<std::string> args;
    for (size_t i = 2; i < params.size(); ++i)
        args.push_back(params[i]);

    bool adding = true;
    size_t argIndex = 0;

    std::string addModeStr;
    std::string removeModeStr;

    for (size_t i = 0; i < modeStr.size(); i++)
    {
        char f = modeStr[i];
        if (f == '+') {
            adding = true;
            continue;
        }
        if (f == '-') {
            adding = false;
            continue;
        }

        switch (f) {
        case 'i':
            ch->setI(adding);
            if (adding) addModeStr += "i";
            else removeModeStr += "i";
            break;
        case 't':
            ch->setT(adding);
            if (adding) addModeStr += "t";
            else removeModeStr += "t";
            break;
        case 'k':
            if (adding) {
                if (ch->isK())
                {
                    sendError(c, ERR_KEYSET, "", ch->getName());
                    continue;
                }
                if (argIndex >= args.size()) {
                    sendError(c, ERR_NEEDMOREPARAMS, "MODE", "");
                    continue;
                }
                std::string pass = args[argIndex++];
                _parser.trim(pass);
                if (pass.empty())
                {
                    sendError(c, ERR_NEEDMOREPARAMS, "MODE", "");
                    continue;
                }
                ch->setK(true, pass);
            }
            else {
                ch->setK(false, "");
            }
            if (adding) addModeStr += "k";
            else removeModeStr += "k";
            break;
        case 'l':
            if (adding) {
                if (argIndex >= args.size()) {
                    sendError(c, ERR_NEEDMOREPARAMS, "MODE", "");
                    continue;
                }
                std::string limRaw = args[argIndex++];
                _parser.trim(limRaw);
                // TODO: chech limRaw int NUMBER correct ????
                int lim;
                std::istringstream(limRaw) >> lim;
                ch->setL(lim);
            }
            else {
                ch->setL(-1);
            }
            if (adding) addModeStr += "l";
            else removeModeStr += "l";
            break;
        case 'o':
            if (argIndex >= args.size()) {
                sendError(c, ERR_NEEDMOREPARAMS, "MODE", "");
                continue;
            }
            else {
                std::string nick = args[argIndex++];
                _parser.trim(nick);
                if (!_parser.isValidNick(nick))
                {
                    sendError(c, ERR_NOSUCHNICK, nick, "");
                    continue;
                }
                std::string nickLower = _parser.ircLowerStr(nick);
                Client *user = getClientByNick(nickLower);
                if (!user) {
                    sendError(c, ERR_NOSUCHNICK, nick, "");
                    continue;
                }
                if (adding)
                    ch->addOperator(user->getNickLower());
                else
                    ch->removeOperator(user->getNickLower());
            }
            break;
        default:
            sendError(c, ERR_UNKNOWNMODE, std::string(1, f) , "");
            continue;
        }
    }

    if (addModeStr.empty() && removeModeStr.empty()) return;
    
    if (!addModeStr.empty()) addModeStr = " +" + addModeStr;
    if (!removeModeStr.empty()) removeModeStr = " -" + removeModeStr;

    std::string modeMsg = ":" + c->buildPrefix() + " MODE " + ch->getName() + addModeStr + removeModeStr;
    for (size_t i = 0; i < argIndex; i++)
        modeMsg += " " + args[i];
    modeMsg += "\r\n";

    c->enqueue_reply(modeMsg);
    set_event_for_sending_msg(c->getFD(), true);
    ch->broadcast(c, modeMsg);
    set_event_for_group_members(ch, true);
}
