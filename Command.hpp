#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <iostream>

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
    CAP, //??,
    PING,
};

class Command
{
private:
    Commands _command;
    std::string _mode;
    std::string _text;

public:
    Command() : _command(NOT_FOUND), _mode(""), _text("") {};
    Command(const Commands &command, const std::string &mode, const std::string &text)
    {
        _command = command;
        _mode = mode;
        _text = text;
    };
    Command(const Commands &command)
    {
        _command = command;
    };

    void setCommand(const Commands &command);
    void setMode(const std::string &mode);
    void setText(const std::string &text);

    const Commands &getCommand();
    const std::string &getMode() const;
    const std::string &getText() const;
};

#endif