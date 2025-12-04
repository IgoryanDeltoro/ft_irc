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
    std::cout << "JOIN Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.size() < 2)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "KICK");
        return;
    }

    std::vector<std::string> channelNames = _parser.splitByComma(params[0]);
    std::vector<std::string> namesToKick = _parser.splitByComma(params[1]);

    if (channelNames.size() != namesToKick.size())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "KICK");
        return;
    }

    // check channels     # || &      && > 2
    for (size_t i = 0; i < channelNames.size(); i++)
    {
        if (channelNames[i].size() < 2)
        {
            // error
            return;
        }
    }
    // check userNames size > 0
    for (size_t i = 0; i < namesToKick.size(); i++)
    {
        if (namesToKick[i].size() < 1)
        {
            //error
            return;
        }
    }

    // для каждого channel и userNames
    for (size_t i = 0; i < namesToKick.size(); i++)
    {
        Channel *ch;
        if (!_channels.count(channelNames[i]))
        {
            sendError(c, ERR_NOSUCHCHANNEL, channelNames[i]);
            continue;;
        }
        ch = _channels[channelNames[i]];
        if (!ch->isUser(c->getNick()))
        {
            sendError(c, ERR_NOTONCHANNEL, channelNames[i]);
            return;
        }
        if (!ch->getOperators().count(c->getNick()))
        {
            sendError(c, ERR_CHANOPRIVSNEEDED, channelNames[i]);
            return;
        }

        // проверяем ник в канале
        if (!ch->isUser(namesToKick[i]))
        {
            sendError(c, ERR_USERNOTINCHANNEL, namesToKick[i]);
            return;
        }

        Client *userToKick = NULL;
        for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); it++)
        {
            if (it->second->getNick() == namesToKick[i])
            {
                userToKick = it->second;
                break;
            }
        }
        if (!userToKick)
        {
            //error
        }

        // fds to send users ( + deletet user)
        // std::vector<int> fds;
        // std::set<std::string> &chUsers = ch->getUsers();
        // for (std::map<std::string, Client *>::iterator it = chUsers.begin(); it != chUsers.end(); it++)
        // {
        //     fds.push_back(it->second->getFD());
        // }
        //удаляем
        // kickClientFromChannel(*ch, userToKick);
        // сообщаем всем fds
        // std::string outMessage = ":" + c->getNick() + " KICK " + channelNames[i] + " " + userToKick->getNick() + " :" + command.getText() + "\r\n";
        // for (size_t d = 0; d < fds.size(); i++)
        // {
        //     send_message_to_client(c->getFD(), outMessage);
        // }
    }

    //         Examples :
    //         : Angel INVITE Wiz #Dust; User Angel inviting WiZ to channel #Dust
    //         INVITE Wiz #Twilight_Zone; Command to invite WiZ to #Twilight_zone
}

void Server::kickClientFromChannel(Channel &channel, Client *client)
{
    const std::string kickedNick = client->getNick();
    std::map<std::string, Client*> &users = channel.getUsers();
    std::set<std::string> &operators = channel.getOperators();
    std::set<std::string> &invited = channel.getInvited();

    users.erase(client->getNick());
    operators.erase(client->getNick());
    invited.erase(client->getNick());
}
