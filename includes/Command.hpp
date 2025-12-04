#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <iostream>
#include <vector>

enum Commands
{
    NOT_FOUND,
    HELP,
    PASS,
    NICK,
    USER,
    JOIN,
    INVITE,
    KICK,
    TOPIC,
    MODE,
    PRIVMSG,
    QUIT,
    LIST,
    CAP,
    PING,
    // PART
    //    NOTICE
};

class Command
{
private:
    std::string _prefix;
    Commands _command;
    std::vector<std::string> _params;
    std::string _text;
    Command(const Commands &command);
    Command &operator=(const Commands &command);

public:
    Command();
    ~Command();

    void setPrefix(const std::string &);
    void setCommand(const Commands &);
    void addParam(const std::string &);
    void setText(const std::string &);

    const std::string& getPrefix() const;
    const Commands &getCommand();
    const std::vector<std::string> &getParams() const;
    const std::string &getText() const;
};

#endif