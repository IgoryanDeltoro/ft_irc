#include "../includes/Parser.hpp"

Parser::Parser() {}
Parser::~Parser() {}

// <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
// <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
// <command>  ::= <letter> { <letter> } | <number> <number> <number>
// <SPACE>    ::= ' ' { ' ' }
// <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
// <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
//                or NUL or CR or LF, the first of which may not be ':'>
// <trailing> ::= <Any, possibly *empty*, sequence of octets not including
//                  NUL or CR or LF>
// <crlf>     ::= CR LF
//   <target>     ::= <to> [ "," <target> ]
//    <to>         ::= <channel> | <user> '@' <servername> | <nick> | <mask>
//    <servername> ::= <host>
//    <host>       ::= see RFC 952 [DNS:4] for details on allowed hostnames
//    <mask>       ::= ('#' | '$') <chstring>

Command Parser::parse(std::string &line) const
{
    Command cmd;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\0' || c == '\r' || c == '\n') return cmd;
    }

    // ---------- 1. Parse prefix ----------
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

    // ---------- 2. Parse command ----------
    if (!(ss >> token)) return cmd;
    
    std::string cmdStr = token;
    for (size_t i = 0; i < cmdStr.size(); i++) {
        cmdStr[i] = std::toupper(cmdStr[i]);
    }

    cmd.setCommand(mapCommand(cmdStr), cmdStr);

    // ---------- 3. Parse parameters ----------
    int paramCount = 0;
    while (ss >> token) {
        if (token[0] == ':') {
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
    return c == '-' || c == '[' || c == ']' || c == '\\' || c == '`' || c == '^' || c == '{' || c == '}';
}

bool Parser::isNonWhite(char c) const {
    return c != ' ' && c != '\0' && c != '\r' && c != '\n';
}

bool Parser::isChstring(char c) const {
    return c != ' ' && c != ',' && c != '\x07' && c != '\0' && c != '\r' && c != '\n';
}

bool Parser::isValidChstring(const std::string &str) const
{
    if (str.empty())
        return false;
    for (size_t i = 1; i < str.size(); ++i) {
        if (!isChstring(str[i]))
            return false;
    }
    return true;
}

bool Parser::isValidNick(const std::string &nick) const
{
    if (nick.empty() || nick.size() > 9)
        return false;
    if (!isLetter(nick[0]))
        return false;
    for (size_t i = 1; i < nick.size(); ++i) {
        if (!(isLetter(nick[i]) || isNumber(nick[i]) || isSpecial(nick[i])))
            return false;
    }
    return true;
}

bool Parser::isValidChannelName(const std::string &name) const
{
    if (name.size() < 2 || name.size() > 200)
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
    if (cmd == "PASS")
        return PASS;
    if (cmd == "NICK")
        return NICK;
    if (cmd == "USER")
        return USER;
    if (cmd == "JOIN")
        return JOIN;
    if (cmd == "MODE")
        return MODE;
    if (cmd == "KICK")
        return KICK;
    if (cmd == "TOPIC")
        return TOPIC;
    if (cmd == "INVITE")
        return INVITE;
    if (cmd == "PRIVMSG")
        return PRIVMSG;
    if (cmd == "QUIT")
        return QUIT;
    if (cmd == "LIST")
        return LIST;
    if (cmd == "CAP")
        return CAP;
    if (cmd == "PING")
        return PING;
    if (cmd == "HELP")
        return HELP;
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

// bool hasWildcard(const std::string &s) {
//     return s.find('*') != std::string::npos || s.find('?') != std::string::npos;
// }

// bool wildcardAfterLastDot(const std::string &s)
// {
//     size_t dot = s.find_last_of('.');
//     if (dot == std::string::npos)
//         return true;
//     for (size_t i = dot + 1; i < s.size(); ++i)
//         if (s[i] == '*' || s[i] == '?')
//             return true;
//     return false;
// }

// bool matchMask(const std::string &mask, const std::string &text)
// {
//     size_t m = 0, t = 0, star = std::string::npos, match = 0;

//     while (t < text.size()) {
//         if (m < mask.size() &&
//             (mask[m] == '?' || mask[m] == text[t])) {
//             ++m;++t;
//         }
//         else if (m < mask.size() && mask[m] == '*') {
//             star = m++;
//             match = t;
//         }
//         else if (star != std::string::npos) {
//             m = star + 1;
//             t = ++match;
//         }
//         else
//             return false;
//     }

//     while (m < mask.size() && mask[m] == '*')
//         ++m;

//     return m == mask.size();
// }

// bool Parser::isValidHost(const std::string &str) const {
//     if (str.empty())
//         return false;
//     for (size_t i = 0; i < str.size(); ++i) {
//         char c = str[i];
//         if (!(isLetter(c) || isNumber(c) || c == '.' || c == '-'))
//             return false;
//     }
//     return true;
// }

// bool Parser::isValidMask(const std::string &s) const {
//     if (s.size() < 2)
//         return false;
//     if (s[0] != '#' && s[0] != '$')
//         return false;
//     if (!isValidChstring(s.substr(1)))
//         return false;

//     // защита от #* и $*
//     if (s == "#*" || s == "$*")
//         return false;

//     return true;
// }