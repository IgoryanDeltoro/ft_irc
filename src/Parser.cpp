#include "../includes/Parser.hpp"

Parser::Parser() {}
Parser::~Parser() {}

Command Parser::parse(std::string &line) const
{

    Command cmd;

    trim(line);
    if (line.empty())
        return cmd;

    std::stringstream ss(line);
    std::string token;

    // ---------- 1. Parse prefix ----------
    if (line[0] == ':') {
        ss >> token;                    // token = ":prefix" ":nick!user@host"
        cmd.setPrefix(token.substr(1)); // remove ':'
    }

    // ---------- 2. Parse command ----------
    ss >> token;
    std::string cmdStr = token;

    for (size_t i = 0; i < cmdStr.size(); i++) {
        cmdStr[i] = std::toupper(cmdStr[i]);
    }

    cmd.setCommand(mapCommand(cmdStr));

    // ---------- 3. Parse parameters ----------
    while (ss >> token)
    {
        if (token[0] == ':')
        {
            // It's trailing — grab rest of stream (включая пробелы)
            std::string trailing = token.substr(1);
            std::string rest;
            std::getline(ss, rest);
            trailing += rest;
            cmd.setText(trailing);
            break;
        }
        else
        {
            cmd.addParam(token);
        }
    }
    return cmd;
}

void Parser::trim(std::string &s) const
{
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");

    if (start == std::string::npos)
        s.clear();
    else
        s = s.substr(start, end - start + 1);
}

std::vector<std::string> Parser::splitByComma(const std::string &s)
{
    std::vector<std::string> splited;
    std::string tmp;

    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == ',')
        {
            trim(tmp);
            if (!tmp.empty())
                splited.push_back(tmp);
            tmp.clear();
        }
        else
        {
            tmp += s[i];
        }
    }
    trim(tmp);
    if (!tmp.empty())
        splited.push_back(tmp);
    return splited;
}

bool Parser::isValidNick(const std::string &nick) const
{
    if (nick.empty() || nick.size() > 9)
        return false;

    const std::string forbiddenFirst = "$:#&~@%+";
    const std::string forbidden = " ,*?!@";

    if (forbiddenFirst.find(nick[0]) != std::string::npos)
        return false;

    for (size_t i = 0; i < nick.size(); i++)
    {
        if (forbidden.find(nick[i]) != std::string::npos)
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

    for (size_t i = 1; i < name.size(); ++i)
    {
        if (name[i] == ' ' ||    // space
            name[i] == ',' ||    // comma
            name[i] == '\x07' || // BEL (ASCII 7)
            name[i] == '\0' ||   // NUL
            name[i] == '\r' ||   // CR
            name[i] == '\t' ||   // TAB
            name[i] == '\n')     // LF
        {
            return false;
        }
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
