#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <sstream>

#include "Client.hpp"

class Client;

class Parser {
    private:
        Parser(const Parser &);
        Parser &operator=(const Parser &);

        void trim(std::string &s) const;
    public:
        Parser();
        ~Parser();

        void parse(Client &c, std::string &line) const;
};

#endif