#include "../../includes/Server.hpp"

void Server::topic(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    std::vector<std::string> params = command.getParams();

std::cout << "TOPIC Command params:" << std::endl;
    for (size_t i = 0; i < params.size(); i++)
        std::cout << i << ": " << params[i] << std::endl;

    if (params.size() < 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "TOPIC");
        return;
    }
    std::string channelName = params[0];

    if (channelName.empty() || channelName.size() < 2 || (channelName[0] != '#' && channelName[0] != '&'))
    {
        sendError(c, ERR_NOSUCHCHANNEL, channelName);//TODO ERROR wrong arg ?
        return;
    }

    //TODO!!! check chanelName

    Channel *ch;
    if (_channels.count(channelName) == 0)
    {
        sendError(c, ERR_NOSUCHCHANNEL, channelName);
        return;
    }
    ch = _channels[channelName];

    if (!ch->isUser(c->getNick()))
    {
        sendError(c, ERR_NOTONCHANNEL, channelName);
        return;
    }

    if (command.getText().empty())
    {
        if (ch->getTopic().empty())
            sendError(c, RPL_NOTOPIC, channelName);
        else
        {
            std::string msg = channelName + " :" + ch->getTopic() + "\r\n"; // 332 RPL_TOPIC "<channel> :<topic>"
            send_message_to_client(c->getFD(), msg);
        }
    }
    else
    {
        if (ch->isT() && !ch->isOperator(c->getNick()))
        {
            sendError(c, ERR_CHANOPRIVSNEEDED, channelName);
            return;
        }
        if (ch->getTopic().empty())
        {
            ch->setTopic(command.getText());
            std::string msg = ":Topic " + command.getText() + " on " + channelName + " seted\r\n";
            send_message_to_client(c->getFD(), msg);
        }
        else
        {
            std::string old = ch->getTopic();
            ch->setTopic(command.getText());
            std::string msg = ":User " + c->getNick() + " changed topic from " + old + " to " + command.getText() + " on " + channelName + "\r\n";
            send_message_to_client(c->getFD(), msg);
        }
    }
}
