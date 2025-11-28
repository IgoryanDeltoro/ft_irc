#include "Command.hpp"

void Command::setCommand(const Commands &command)
{
    _command = command;
}
void Command::setMode(const std::string &mode)
{
    _mode = mode;
}
void Command::setText(const std::string &text)
{
    _text = text;
}

const Commands &Command::getCommand()
{
    return _command;
}
const std::string &Command::getMode() const
{
    return _mode;
}
const std::string &Command::getText() const
{
    return _text;
}