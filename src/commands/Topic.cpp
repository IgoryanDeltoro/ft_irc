#include "../../includes/Server.hpp"

void Server::topic(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    std::vector<std::string> params = command.getParams();

    if (params.size() < 1)
    {
        sendNumericReply(c, ERR_NEEDMOREPARAMS, "TOPIC", "");
        return;
    }
    std::string channelName = params[0];
    if (!_parser.isValidChannelName(channelName))
    {
        sendNumericReply(c, ERR_BADCHANMASK, "", channelName);
        return;
    }
    std::string channelNameLower = _parser.ircLowerStr(channelName);

    Channel *ch;
    if (_channels.count(channelNameLower) == 0)
    {
        sendNumericReply(c, ERR_NOSUCHCHANNEL, "", channelName);
        return;
    }
    ch = _channels[channelNameLower];

    if (!ch->isUser(c->getNickLower()))
    {
        sendNumericReply(c, ERR_NOTONCHANNEL, "", channelName);
        return;
    }

    if (command.getText().empty())
    {
        if (!ch->hasTopic())
        {
            // 331 RPL_NOTOPIC
            std::string msg = ":server 331 " + c->getNick() + " " + ch->getName() + " :No topic is set\r\n";

            c->enqueue_reply(msg);
            set_event_for_sending_msg(c->getFD(), true);
        }
        else
        {
            // 332 RPL_TOPIC
            std::string msg1 = ":server 332 " + c->getNick() + " " + ch->getName() + " :" + ch->getTopic() + "\r\n";

            c->enqueue_reply(msg1);
            set_event_for_sending_msg(c->getFD(), true);

            // 333 RPL_TOPICWHOTIME
            std::string msg2 = ":server 333 " + c->getNick() + " " + ch->getName() + " " + ch->getTopicSetter() + " " + std::to_string(ch->getTopicTimestamp()) + "\r\n";

            c->enqueue_reply(msg2);
            set_event_for_sending_msg(c->getFD(), true);
        }
        return;
    }

    if (ch->isT() && !ch->isOperator(c->getNickLower()))
    {
        sendNumericReply(c, ERR_CHANOPRIVSNEEDED, "", channelName);
        return;
    }

    std::string newTopic = command.getText();
    ch->setTopic(newTopic, c->getNick());

    std::string msg = c->buildPrefix() + " TOPIC " + ch->getName() + " :" + newTopic + "\r\n";
    ch->broadcast(NULL, msg);
    set_event_for_group_members(ch, true);
}
