#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <iostream>
#include <vector>

enum Commands
{
    NOT_VALID,
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
    PING
};

class Command
{
private:
    std::string                     _prefix;
    std::string                     _commandStr;
    std::string                     _text;
    Commands                        _command;
    std::vector<std::string>        _params;

    Command(const Commands &command);
    Command &operator=(const Commands &command);

public:
    Command();
    ~Command();

    void                            setPrefix(const std::string &);
    void                            setCommand(const Commands &, const std::string &str);
    void                            addParam(const std::string &);
    void                            setText(const std::string &);

    const std::string               &getPrefix() const;
    const Commands                  &getCommand();
    const std::string               &getCommandStr() const;
    const std::string               &getText() const;
    const std::vector<std::string>  &getParams() const;
};

#endif