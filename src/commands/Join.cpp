#include "../../includes/Server.hpp"

void Server::join(Client *c, const Command &command)
{
    if (!isClientAuth(c))
        return;
    // Command: JOIN
    // Parameters: <channel>{,<channel>} [<key>{,<key>}]

    std::vector<std::string> params = command.getParams();
    if (params.empty())
    {
        sendError(c, ERR_NEEDMOREPARAMS, "JOIN");
        return;
    }

    std::vector<std::string> channelNames = _parser.splitByComma(params[0]);
    std::vector<std::string> keys;

    if (params.size() > 1)
    {
        _parser.trim(params[1]);
        keys = _parser.splitByComma(params[1]);
    }

    for (size_t i = 0; i < channelNames.size(); i++)
    {
        std::string chan = channelNames[i];
        std::string key = (i < keys.size() ? keys[i] : "");

        if (!_parser.isValidChannelName(chan))
        {
            sendError(c, ERR_BADCHANMASK, chan);
            continue;
        }

        joinChannel(c, chan, key);
    }
}

void Server::joinChannel(Client *c, const std::string &name, const std::string &password)
{
    Channel *ch;

    if (_channels.count(name) == 0)
    {
        ch = new Channel(name, c);
        if (!password.empty())
        {
            ch->setK(true, password);
        }
        _channels[name] = ch;
    }
    else
    {
        ch = _channels[name];

        if (ch->isI() && !ch->isOperator(c->getNick()))
        {
            sendError(c, ERR_INVITEONLYCHAN, name);
            return;
        }
        if (!ch->getPassword().empty() && ch->getPassword() != password)
        {
            sendError(c, ERR_BADCHANNELKEY, name);
            return;
        }

        // +l (user limit)
        if (ch->getUserLimit() > 0 && (int)ch->getUsers().size() >= ch->getUserLimit())
        {
            sendError(c, ERR_CHANNELISFULL, name);
            return;
        }

        if (ch->isUser(c->getNick()))
        {
            // send_message_to_client(c->getFD(), ":Channel " + name + " USER already in channel\r\n");
            return;
        }


        ch->addUser(c);
        //:nick!user@host JOIN #channel
    }

    //for all in channel
    
    c->enqueue_reply(":" + c->getNick() + "!" + c->getUserName() + "@" + "<host>" +" :END!\r\n");

    set_event_for_sending_msg(c->getFD());

    // • При успешном join отправлять :
	// • RPL_TOPIC — тема канала
    //332 RPL_TOPIC
        //"<channel> :<topic>"

	// • RPL_NAMREPLY — список пользователей на канале
    //353 RPL_NAMREPLY
    // "<channel> :[[@|+]<nick> [[@|+]<nick> [...]]]"
    //:nick!user@host JOIN #channel


    //Ты должен отправить список пользователей:
    //:server 353 nick = #chan :@op1 +voiced1 user3 ...
    //:server 366 nick #chan :End of /NAMES list.
}
