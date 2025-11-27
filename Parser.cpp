#include "Parser.hpp"

Parser::Parser() {}
Parser::~Parser() {}



void Parser::parse(Client &c, std::string &line) const {

    trim(line);
    if (line.empty())
    {

    }

    std::vector<std::string> tokens;
    std::stringstream ss(line);

  	std::string word;
    while(getline(ss, word, ' ')){
        tokens.push_back(word);
    }

    if (tokens.front() == "HELP")
    {

    }
    // else if (tokens.front() == "PASS");



    // if (!c.getPassStatus())
    // {
        
    // }


    // else
    // {

    // }

    // if (!c.getPassStatus())
    //     {

    // }
    // else
    // {
        
    // }
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