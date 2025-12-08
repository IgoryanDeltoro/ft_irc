#include "../../includes/Server.hpp"

void Server::mode(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    // Parameters:
    //  <channel> {[+| -] | o | p | s | i | t | n | b | v}[<limit>][<user>]
    std::vector<std::string> params = command.getParams();
    if (params.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "MODE");
        return;
    }
    std::string target = params[0]; // канал
    if (!target.empty() && !(target[0] == '#' || target[0] == '&'))
        return;

    if (!_parser.isValidChannelName(target))
    {
        sendError(c, ERR_BADCHANMASK, target);
        return;
    }
    std::string channelNameLower = _parser.ircLowerStr(target);

    Channel *ch = NULL;

    if (_channels.count(channelNameLower) == 0)
    {
        sendError(c, ERR_NOSUCHCHANNEL, target);
        return;
    }
    ch = _channels[channelNameLower];

    if (!ch->isUser(c->getNickLower()))
    {
        sendError(c, ERR_NOTONCHANNEL, target);
        return;
    }
    if (!ch->isOperator(c->getNickLower()))
    {
        sendError(c, ERR_CHANOPRIVSNEEDED, target);
        return;
    }

    std::string modeStr = (params.size() > 1 ? params[1] : "");
    std::vector<std::string> args;
    for (size_t i = 2; i < params.size(); ++i)
        args.push_back(params[i]);

    bool adding = true;
    size_t argIndex = 0;

    for (size_t i = 0; i < modeStr.size(); i++)
    {
        char f = modeStr[i];
        if (f == '+')
        {
            adding = true;
            continue;
        }
        if (f == '-')
        {
            adding = false;
            continue;
        }

        switch (f)
        {
        case 'i':
            ch->setI(adding);
            break;
        case 't':
            ch->setT(adding);
            break;
        case 'k':
            if (adding)
            {
                if (argIndex >= args.size())
                {
                    sendError(c, ERR_NEEDMOREPARAMS, "MODE " + target);
                    return;
                }
                ch->setK(true, args[argIndex++]);
            }
            else
            {
                ch->setK(false, "");
            }
            break;
        case 'l':
            if (adding)
            {
                if (argIndex >= args.size())
                {
                    sendError(c, ERR_NEEDMOREPARAMS, "MODE " + target);
                    return;
                }
                int lim;
                std::istringstream(args[argIndex++]) >> lim;
                ch->setL(lim);
            }
            else
            {
                ch->setL(-1);
            }
            break;
        case 'o':
            if (argIndex >= args.size())
            {
                sendError(c, ERR_NEEDMOREPARAMS, "MODE " + target);
                return;
            }
            else
            {
                std::string nick = args[argIndex++];
                std::string nickLower = _parser.ircLowerStr(nick);
                Client *user = getClientByNick(nickLower);
                if (!user)
                {
                    sendError(c, ERR_NOSUCHNICK, args[argIndex - 1]);
                    return;
                }
                if (adding)
                    ch->addOperator(user->getNickLower());
                else
                    ch->removeOperator(user->getNickLower());
            }
            break;
        default:
            sendError(c, ERR_UNKNOWNMODE, std::string(1, f));
            break;
        }
    }
}
