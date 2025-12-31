#include "../includes/Parser.hpp"

Parser::Parser() {}
Parser::~Parser() {}

Command Parser::parse(std::string &line) const
{
    Command cmd;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\0' || c == '\r' || c == '\n') return cmd;
    }

    if (line[0] == ':') {
        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos)
            return cmd;
        if (spacePos == 1)
            return cmd;
        std::string prefix = line.substr(1, spacePos - 1);
        cmd.setPrefix(prefix);
        line = line.substr(spacePos + 1);
    }

    std::stringstream ss(line);
    std::string token;

    if (!(ss >> token)) return cmd;
    
    std::string cmdStr = token;
    for (size_t i = 0; i < cmdStr.size(); i++) {
        cmdStr[i] = std::toupper(static_cast<unsigned char>(cmdStr[i]));
    }

    cmd.setCommand(mapCommand(cmdStr), cmdStr);

    int paramCount = 0;
    while (ss >> token) {
        if (token[0] == ':') {
            if (++paramCount > 15) {
                cmd.setCommand(NOT_VALID, "");
                return cmd;
            }
            std::string trailing = token.substr(1);
            std::string rest;
            std::getline(ss, rest);
            trailing += rest;
            cmd.setText(trailing);
            break;
        }
        else {
            if (++paramCount > 15) {
                cmd.setCommand(NOT_VALID, "");
                return cmd;
            }
            cmd.addParam(token);
        }
    }
    return cmd;
}

const std::vector<std::string> Parser::splitByComma(const std::string &s) const
{
    std::vector<std::string> splited;
    std::string tmp;

    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == ',') {
            if (!tmp.empty())
                splited.push_back(tmp);
            tmp.clear();
        }
        else  tmp += s[i];
    }
    if (!tmp.empty())
        splited.push_back(tmp);
    return splited;
}

bool Parser::isLetter(char c) const {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool Parser::isNumber(char c) const {
    return (c >= '0' && c <= '9');
}

bool Parser::isSpecial(char c) const {
    return c == '[' || c == ']' || c == '\\' || c == '`' ||
           c == '_' || c == '^' || c == '{' || c == '|' || c == '}';
}

bool Parser::isNonWhite(char c) const {
    return c != ' ' && c != '\0' && c != '\r' && c != '\n' && c != '@';
}

bool Parser::isChstring(char c) const {
    return c != ' ' && c != ',' && c != ':' && c != '\x07' && c != '\0' && c != '\r' && c != '\n';
}

bool Parser::isValidChstring(const std::string &str) const
{
    if (str.empty())
        return false;
    for (size_t i = 0; i < str.size(); ++i) {
        if (!isChstring(str[i]))
            return false;
    }
    return true;
}

bool Parser::isValidNick(const std::string &nick) const
{
    if (nick.empty() || nick.size() > 9)
        return false;
    if (!(isLetter(nick[0]) || isSpecial(nick[0])))
        return false;
    for (size_t i = 1; i < nick.size(); ++i) {
        if (!(isLetter(nick[i]) || isNumber(nick[i]) || isSpecial(nick[i]) || nick[i] == '-'))
            return false;
    }
    return true;
}

bool Parser::isValidChannelName(const std::string &name) const
{
    if (name.size() < 2 || name.size() > 50)
        return false;
    if (name[0] != '#' && name[0] != '&')
        return false;
    for (size_t i = 1; i < name.size(); ++i) {
        if (!isChstring(name[i]))
            return false;
    }
    return true;
}

bool Parser::isValidUser(const std::string &str) const {
    if (str.empty())
        return false;
    for (size_t i = 0; i < str.size(); ++i)
        if (!isNonWhite(str[i]))
            return false;
    return true;
}

Commands Parser::mapCommand(const std::string &cmd) const
{
    if (cmd == "PASS") return PASS;
    if (cmd == "NICK") return NICK;
    if (cmd == "USER") return USER;
    if (cmd == "JOIN") return JOIN;
    if (cmd == "MODE") return MODE;
    if (cmd == "KICK") return KICK;
    if (cmd == "TOPIC") return TOPIC;
    if (cmd == "INVITE") return INVITE;
    if (cmd == "PRIVMSG") return PRIVMSG;
    if (cmd == "CAP") return CAP;
    if (cmd == "PING") return PING;
    if (cmd == "HELP") return HELP;
    if (cmd == "AWAY") return AWAY;
    if (cmd == "PART") return PART;
    if (cmd == "QUIT") return QUIT;
    return NOT_FOUND;
}

char Parser::ircLower(char c) const
{
    if (c >= 'A' && c <= 'Z')
        return c + 32;
    if (c == '{')
        return '[';
    if (c == '}')
        return ']';
    if (c == '|')
        return '\\';
    return c;
}

std::string Parser::ircLowerStr(const std::string &s) const
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); i++)
        out[i] = ircLower(out[i]);
    return out;
}
