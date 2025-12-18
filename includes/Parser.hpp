#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <sstream>
#include "Command.hpp"

class Command;

class Parser {
    private:
        Parser(const Parser &);
        Parser &operator=(const Parser &);

        char ircLower(char c) const;
    public:
        Parser();
        ~Parser();

        Command parse(std::string &) const;

        // void trim(std::string &) const;
        const std::vector<std::string> splitByComma(const std::string &);

        Commands mapCommand(const std::string &cmd) const;
        bool isValidNick(const std::string &) const;
        bool isValidChannelName(const std::string &name) const;
        std::string ircLowerStr(const std::string &s) const;
 
};

#endif
