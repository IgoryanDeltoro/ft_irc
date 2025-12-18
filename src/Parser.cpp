#include "../includes/Parser.hpp"

Parser::Parser() {}
Parser::~Parser() {}

Command Parser::parse(std::string &line) const
{
    Command cmd;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\0' || c == '\r' || c == '\n' || c == '\x07') return cmd;
    }

    std::stringstream ss(line);
    std::string token;

    // ---------- 1. Parse prefix ----------
    if (line[0] == ':') {
        ss >> token;
        cmd.setPrefix(token.substr(1));
    }

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
            for (size_t i = 0; i < token.size(); i++) {
                char c = token[i];
                if (c == ' ' || c == '\t' || c == '\0' || c == '\r' || c == '\n' || c == '\x07')
                {
                    cmd.setCommand(NOT_VALID, "");
                    return cmd;
                }
            }
            if (++paramCount > 15) {
                cmd.setCommand(NOT_VALID, "");
                return cmd;
            }
            cmd.addParam(token);
        }
    }
    return cmd;
}

// void Parser::trim(std::string &s) const
// {
//     size_t start = s.find_first_not_of(" ");
//     size_t end = s.find_last_not_of(" ");

//     if (start == std::string::npos)
//         s.clear();
//     else
//         s = s.substr(start, end - start + 1);
// }

const std::vector<std::string> Parser::splitByComma(const std::string &s)
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

bool Parser::isValidNick(const std::string &nick) const
{
    if (nick.empty() || nick.size() > 9) return false;
    if (!std::isalpha(nick[0])) return false;
    for (size_t i = 1; i < nick.size(); i++) {
        char c = nick[i];
        if (!(std::isalpha(c) || std::isdigit(c) || c == '-' || c == '[' || c == ']' || c == '\\' || c == '`' || c == '^' || c == '{' || c == '}')) return false;
    }
    return true;
}

bool Parser::isValidChannelName(const std::string &name) const
{
    if (name.size() < 2 || name.size() > 200) return false;
    if (name[0] != '#' && name[0] != '&') return false;
    for (size_t i = 1; i < name.size(); ++i) {
        if (name[i] == ' ' || name[i] == ',' || name[i] == '\x07' ||  name[i] == '\0' || name[i] == '\r' || name[i] == '\n') return false;
    }
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
