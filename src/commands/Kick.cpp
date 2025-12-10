#include "../../includes/Server.hpp"

void Server::kick(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;

    //     ERR_NEEDMOREPARAMS ERR_NOSUCHCHANNEL
    //         ERR_BADCHANMASK ERR_CHANOPRIVSNEEDED
    //             ERR_NOTONCHANNEL

    //                 Examples :

    // KICK &Melbourne Matthew; Kick Matthew from &Melbourne
    // KICK #Finnish John : Speaking English; Kick John from #Finnish using "Speaking English" as the reason(comment).
    // : WiZ KICK #Finnish John; KICK message from WiZ to remove John from channel #Finnish
    //  NOTE : It is possible to extend the KICK command parameters to the following :
    // <channel> <user> [<comment>]
    // <channel>{, <channel>} < user>{, <user>}[<comment>]

    std::vector<std::string> params = command.getParams();
 
    if (params.size() < 2)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "KICK", "");
        return;
    }

    std::string channelNamesRaw = params[0];
    _parser.trim(channelNamesRaw);
    std::string namesToKickRaw = params[1];
    _parser.trim(namesToKickRaw);

    std::vector<std::string> channelNames = _parser.splitByComma(channelNamesRaw);
    std::vector<std::string> namesToKick = _parser.splitByComma(namesToKickRaw);

    for (size_t i = 0; i < channelNames.size(); i++)
    {
        if (!_parser.isValidChannelName(channelNames[i]) )
        {
            sendError(c, ERR_BADCHANMASK, "", channelNames[i]);
            continue;
        }
        if (i >= namesToKick.size())
        {
            sendError(c, ERR_NEEDMOREPARAMS, "KICK", "");
            continue;
        }

        if (_parser.isValidNick(namesToKick[i]))
        {
            sendError(c, ERR_NOSUCHNICK, namesToKick[i], "");
            return;
        }

        std::string channelNameLower = _parser.ircLowerStr(channelNames[i]);
        Channel *ch;
        if (!_channels.count(channelNameLower))
        {
            sendError(c, ERR_NOSUCHCHANNEL, "", channelNames[i]);
            continue;
        }
        ch = _channels[channelNameLower];
        if (!ch->isUser(c->getNickLower()))
        {
            sendError(c, ERR_NOTONCHANNEL, "", channelNames[i]);
            return;
        }
        if (!ch->getOperators().count(c->getNickLower()))
        {
            sendError(c, ERR_CHANOPRIVSNEEDED, "", channelNames[i]);
            return;
        }
        
        // проверяем ник в канале
        std::string nameToKickLower = _parser.ircLowerStr(namesToKick[i]);

        Client *userToKick = ch->getUser(nameToKickLower);
        if (!userToKick)
        {
            sendError(c, ERR_USERNOTINCHANNEL, namesToKick[i], channelNames[i]);
            return;
        }

        ch->removeInvite(nameToKickLower);
        ch->removeOperator(nameToKickLower);
        ch->removeUser(nameToKickLower);

        std::string outMessage = ":" + c->getNick() + " KICK " + channelNames[i] + " " + userToKick->getNick() + " :" + command.getText() + "\r\n";

        c->enqueue_reply(outMessage);
        set_event_for_sending_msg(userToKick->getFD(), true);
        ch->broadcast(c, outMessage);
        set_event_for_group_members(ch, true);
    }
}
