#include "../includes/Command.hpp"

Command::Command() : _command(NOT_VALID), _hasTraling(false), _hasPrefix(false) {};

Command::~Command() {};

void Command::setPrefix(const std::string &p) { 
    _prefix = p;
    _hasPrefix = true;
}

void Command::setCommand(const Commands &command, const std::string &str)
{
    _command = command;
    _commandStr = str;
}
void Command::addParam(const std::string &param) { _params.push_back(param); }

void Command::setText(const std::string &text) { 
    _text = text;
    _hasTraling = true;
}

const Commands &Command::getCommand() { return _command; }

const std::vector<std::string> &Command::getParams() const { return _params; }

const std::string &Command::getText() const { return _text; }

const std::string& Command::getPrefix() const { return _prefix; }

const std::string &Command::getCommandStr() const { return _commandStr; }

bool Command::hasTraling() const { return _hasTraling; }

bool Command::hasPrefixg() const { return _hasPrefix; }
