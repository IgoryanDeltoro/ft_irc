#include "Parser.hpp"

Parser::Parser() {}
Parser::~Parser() {}

Command Parser::parse(std::string &line) const
{

    Command cmd;
    std::stringstream ss(line);
    std::string token;

    // ---------- 1. Parse prefix ----------
    if (line[0] == ':')
    {
        ss >> token;                   // token = ":prefix"
        cmd.setPrefix(token.substr(1)); // remove ':'
    }

    // ---------- 2. Parse command ----------
    ss >> token;
    std::string cmdStr = token;

    for (size_t i = 0; i < cmdStr.size(); i++)
    {
        cmdStr[i] = std::toupper(cmdStr[i]);
    }

    if (cmdStr == "HELP") cmd.setCommand(HELP);
    else if (cmdStr == "PASS") cmd.setCommand(PASS);
    else if (cmdStr == "NICK") cmd.setCommand(NICK);
    else if (cmdStr == "USER") cmd.setCommand(USER);
    else if (cmdStr == "JOIN") cmd.setCommand(JOIN);
    else if (cmdStr == "MODE") cmd.setCommand(MODE);
    else if (cmdStr == "KICK") cmd.setCommand(KICK);
    else if (cmdStr == "TOPIC") cmd.setCommand(TOPIC);
    else if (cmdStr == "INVITE") cmd.setCommand(INVITE);
    else if (cmdStr == "PRIVMSG") cmd.setCommand(PRIVMSG);
    else if (cmdStr == "LIST") cmd.setCommand(LIST);
    else if (cmdStr == "QUIT") cmd.setCommand(QUIT);
    else if (cmdStr == "CAP") cmd.setCommand(CAP);
    else if (cmdStr == "PING") cmd.setCommand(PING);
    else cmd.setCommand(NOT_FOUND);

    // ---------- 3. Parse parameters ----------
    while (ss >> token)
    {
        if (token[0] == ':')
        {
            // It's trailing — grab rest of stream
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

void Parser::trim(std::string &s) const {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");

    if (start == std::string::npos)
        s.clear();
    else
        s = s.substr(start, end - start + 1);
}

std::vector<std::string> Parser::splitParams(const std::string &str) const
{
    std::vector<std::string> out;
    std::stringstream ss(str);
    std::string word;

    while (ss >> word)
    {
        out.push_back(word);
    }
    return out;
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
    if (nick.empty())
        return false;
    if (!isalpha(nick[0]) && nick[0] != '_')
        return false;

    for (size_t i = 1; i < nick.size(); i++)
    {
        if (!isalnum(nick[i]) &&
            nick[i] != '_' &&
            nick[i] != '-')
            return false;
    }
    return true;
}