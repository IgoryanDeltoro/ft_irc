#include "../includes/Server.hpp"

void Server::sendNumericReply(Client *c, NumericReply r, const std::string &arg, const std::string &ch)
{
    std::string message = getNumericReplyText(r);
    std::string nick = c->getNick().empty() ? "*" : c->getNick();

    size_t pos;
    if ((pos = message.find("<command>")) != std::string::npos) message.replace(pos, 9, arg);
    else if ((pos = message.find("<nick>")) != std::string::npos) message.replace(pos, 6, arg);
    else if ((pos = message.find("<char>")) != std::string::npos) message.replace(pos, 6, arg);
    else if ((pos = message.find("<user>")) != std::string::npos) message.replace(pos, 6, arg);
    else if ((pos = message.find("<target>")) != std::string::npos) message.replace(pos, 8, arg);
    else if ((pos = message.find("<mask>")) != std::string::npos) message.replace(pos, 6, arg);

    if ((pos = message.find("<channel>")) != std::string::npos) message.replace(pos, 9, ch);

    std::string num;
    std::stringstream out;
    out << r;
    num = out.str();

    c->enqueue_reply( ":server " + num + " " + nick + " " + message + "\r\n");
    set_event_for_sending_msg(c->getFD(), true);
}

std::string Server::getNumericReplyText(const NumericReply &r)
{
    switch (r)
    {
    case RPL_CHANNELMODEIS: return "<channel> <nick>";
    case RPL_INVITING: return "<channel> <nick>";
    case RPL_ENDOFNAMES: return "<channel> :End of /NAMES list";
    case RPL_NAMREPLY: return "= <channel> :<nick>";
    case RPL_TOPIC: return "<channel> :<topic>";
    case ERR_KEYSET: return "<channel> :Channel key already set";
    case ERR_UNKNOWNMODE: return "<char> :is unknown mode char to me";
    case ERR_USERONCHANNEL: return "<user> <channel> :is already on channel";
    case ERR_USERNOTINCHANNEL: return "<nick> <channel> :They aren't on that channel";
    case ERR_CHANOPRIVSNEEDED: return "<channel> :You're not channel operator";
    case RPL_NOTOPIC: return "<channel> :No topic is set";
    case ERR_NOTONCHANNEL: return "<channel> :You're not on that channel";
    // case ERR_NOTREGISTERED: return ": User has not registration";
    case ERR_BANNEDFROMCHAN: return "<channel> :Cannot join channel (+b)";
    case ERR_INVITEONLYCHAN: return "<channel> :Cannot join channel (+i)";
    case ERR_BADCHANNELKEY: return "<channel> :Cannot join channel (+k)";
    case ERR_CHANNELISFULL: return "<channel> :Cannot join channel (+l)";
    case ERR_BADCHANMASK: return "<channel> :Invalid channel name";
    case ERR_NOSUCHCHANNEL: return "<channel> :No such channel";
    case ERR_TOOMANYCHANNELS: return "<channel> :You have joined too many channels";
    case ERR_UNKNOWNCOMMAND:
        return "<command> :Unknown command";
    // case ERR_PASSALREADY: return ":Password already success";
    // case ERR_NEEDPASS: return ":Server PASS required ";
    // case ERR_PASSWDMISMATCH: return ":Password incorrect"; //"<client> :Password incorrect"
    case ERR_ALREADYREGISTRED: return ":You may not reregister";
    case ERR_NEEDMOREPARAMS: return "<command> :Not enough parameters";
    case ERR_NONICKNAMEGIVEN: return ":No nickname given";
    case ERR_ERRONEUSNICKNAME: return "<nick> :Erroneus nickname";
    case ERR_NICKNAMEINUSE: return "<nick> :Nickname is already in use";
    case ERR_NORECIPIENT: return "<nick> :No recipient given (PRIVMSG)";
    case ERR_NOTEXTTOSEND: return "<nick> :No text to send";
    case ERR_CANNOTSENDTOCHAN: return "<channel> :Cannot send to channel";
    case ERR_NOTOPLEVEL: return "<mask> :No toplevel domain specified"; // Если клиент отправляет PRIVMSG на некорректный канал/хост.
    case ERR_TOOMANYTARGETS: return "<target> :Duplicate recipients. No message delivered"; //<target> — это первый из дублирующихся или превышающих лимит получателей.
    case ERR_NOSUCHNICK: return "<nick> :No such nick/channel";
    default: return "";
    }
}
