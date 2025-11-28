#include "Parser.hpp"

Parser::Parser() {}
Parser::~Parser() {}

Command Parser::parse(Client &c, std::string &line) const
{
    (void)c;
    

    std::stringstream ss(line);
    std::string cmdStr;
    ss >> cmdStr;

    std::cout << "PARSE cmdStr: " << cmdStr << std::endl;
    Command command;
    if (cmdStr == "HELP")
        command.setCommand(HELP);
    else if (cmdStr == "PASS")
        command.setCommand(PASS);
    else if (cmdStr == "NICK")
        command.setCommand(NICK);
    else if (cmdStr == "USER")
        command.setCommand(USER);
    else if (cmdStr == "JOIN")
        command.setCommand(JOIN);
    else if (cmdStr == "MODE")
        command.setCommand(MODE);
    else if (cmdStr == "KICK")
        command.setCommand(KICK);
    else if (cmdStr == "TOPIC")
        command.setCommand(TOPIC);
    else if (cmdStr == "INVITE")
        command.setCommand(INVITE);
    else if (cmdStr == "PRIVMSG")
        command.setCommand(PRIVMSG);
    else if (cmdStr == "LIST")
        command.setCommand(LIST);
    else if (cmdStr == "QUIT")
        command.setCommand(QUIT);
    else if (cmdStr == "CAP")
        command.setCommand(CAP);
    else
        command.setCommand(NOT_FOUND);
    
    size_t pos = line.find(" :");
    if (pos != std::string::npos)
    {
        std::cout << "PARSE found :" << std::endl;

        std::string mode = line.substr(cmdStr.size(), pos - cmdStr.size());
        trim(mode);
        command.setMode(mode);

        std::string text = line.substr(pos + 2);
        command.setText(text);

        std::cout << "PARSE mode: " << mode << std::endl;
        std::cout << "PARSE text: " << text << std::endl;
    }
    else
    {
        std::cout << "PARSE NOT found :" << std::endl;

        std::string mode = line.substr(cmdStr.size());
        trim(mode);
        command.setMode(mode);
        std::cout << "PARSE mode: " << mode << std::endl;
    }
    return command;
}

// void parse_line(std::string &line)
// {

// }

// PASS passworf
// NICK nick
// USER _user
// HELP help


// JOIN #ch_name
// INVITE
// KICK
// MODE -it #ch_name


// MSG username message kjhghskjs ksdhf ksjh kj h
// Q username message kjhghskjs ksdhf ksjh kj h

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