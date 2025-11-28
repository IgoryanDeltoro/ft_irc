#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <sstream>

#include "Client.hpp"
#include "Command.hpp"

class Client;
class Command;

class Parser {
    private:
        Parser(const Parser &);
        Parser &operator=(const Parser &);
    public:
        Parser();
        ~Parser();

        Command parse(Client &, std::string &) const;

        void trim(std::string &) const;
        std::vector<std::string> splitParams(const std::string &) const;
        std::vector<std::string> splitByComma(const std::string &);

        bool isValidNick(const std::string &) const;
};

#endif