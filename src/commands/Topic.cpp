#include "../../includes/Server.hpp"

void Server::topic(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    std::vector<std::string> params = command.getParams();

    if (params.size() < 1)
    {
        sendError(c, ERR_NEEDMOREPARAMS, "TOPIC", "");
        return;
    }
    std::string channelName = params[0];

    if (!_parser.isValidChannelName(channelName))
    {
        sendError(c, ERR_NOSUCHCHANNEL, "", channelName);
        return;
    }
    std::string channelNameLower = _parser.ircLowerStr(channelName);

    Channel *ch;
    if (_channels.count(channelNameLower) == 0)
    {
        sendError(c, ERR_NOSUCHCHANNEL, "", channelName);
        return;
    }
    ch = _channels[channelNameLower];

    if (!ch->isUser(c->getNickLower()))
    {
        sendError(c, ERR_NOTONCHANNEL, "", channelName);
        return;
    }

    if (command.getText().empty())
    {
        if (ch->getTopic().empty())
            sendError(c, RPL_NOTOPIC, "", channelName);
        else
        {
            std::string msg = channelName + " :" + ch->getTopic() + "\r\n"; // 332 RPL_TOPIC "<channel> :<topic>"
            c->enqueue_reply(msg);
            set_event_for_sending_msg(c->getFD(), true);
        }
    }
    else
    {
        if (ch->isT() && !ch->isOperator(c->getNickLower()))
        {
            sendError(c, ERR_CHANOPRIVSNEEDED, "", channelName);
            return;
        }
        if (ch->getTopic().empty())
        {
            ch->setTopic(command.getText());
            std::string msg = ":Topic " + command.getText() + " on " + channelName + " seted\r\n";
            c->enqueue_reply(msg);
            set_event_for_sending_msg(c->getFD(), true);
        }
        else
        {
            std::string old = ch->getTopic();
            ch->setTopic(command.getText());
            std::string msg = ":User " + c->getNick() + " changed topic from " + old + " to " + command.getText() + " on " + channelName + "\r\n";
            c->enqueue_reply(msg);
            set_event_for_sending_msg(c->getFD(), true);
            ch->broadcast(c, msg);
            set_event_for_group_members(ch, true);
        }
    }
}
