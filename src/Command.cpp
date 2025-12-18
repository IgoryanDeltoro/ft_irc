#include "../includes/Command.hpp"

Command::Command() : _command(NOT_VALID) {};

Command::~Command() {};

void Command::setPrefix(const std::string &p) { _prefix = p; }

void Command::setCommand(const Commands &command, const std::string &str)
{
    _command = command;
    _commandStr = str;
}
void Command::addParam(const std::string &param) { _params.push_back(param); }

void Command::setText(const std::string &text) { _text = text; }

const Commands &Command::getCommand() { return _command; }

const std::vector<std::string> &Command::getParams() const { return _params; }

const std::string &Command::getText() const { return _text; }

const std::string& Command::getPrefix() const { return _prefix; }

const std::string &Command::getCommandStr() const { return _commandStr; }