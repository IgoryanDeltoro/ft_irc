#ifndef PARSER_HPP
# define PARSER_HPP

# include <iostream>
# include <vector>
# include <sstream>
# include "Command.hpp"

class Command;

class Parser {
    private:
        Parser(const Parser &);
        Parser &operator=(const Parser &);
        char ircLower(char c) const;
    public:
        Parser();
        ~Parser();

        bool                            isValidNick(const std::string &nick) const;
        bool                            isValidChannelName(const std::string &name) const;
        bool                            isValidUser(const std::string &str) const;
        std::string                     ircLowerStr(const std::string &s) const;
        Commands                        mapCommand(const std::string &cmd) const;
        Command                         parse(std::string &) const;
        const std::vector<std::string>  splitByComma(const std::string &) const;
        bool                            isLetter(char c) const;
        bool                            isNumber(char c) const;
        bool                            isSpecial(char c) const;
        bool                            isNonWhite(char c) const;
        bool                            isChstring(char c) const;
        bool                            isValidChstring(const std::string &str) const;

};

#endif
