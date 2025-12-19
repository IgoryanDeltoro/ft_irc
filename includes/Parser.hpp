#ifndef PARSER_HPP
# define PARSER_HPP

# include <iostream>
# include <vector>
# include <sstream>
# include "Command.hpp"

class Command;

class Parser {
    private:
        char                            ircLower(char c) const;

        Parser(const Parser &);
        Parser &operator=(const Parser &);

    public:
        Parser();
        ~Parser();

        bool                            isValidNick(const std::string &) const;
        bool                            isValidChannelName(const std::string &name) const;
        std::string                     ircLowerStr(const std::string &s) const;
        Commands                        mapCommand(const std::string &cmd) const;
        Command                         parse(std::string &) const;
        const std::vector<std::string>  splitByComma(const std::string &);
};

#endif
