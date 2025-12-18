#include "../includes/Server.hpp"

bool checkPort(const std::string &str);
bool checkPass(const std::string &password);

int main(int ac, char *av[])
{
    if (ac != 3) {
        std::cerr << "Error: executable example: ./ircserv <port> <password>" << std::endl;
        return 1;
    }
    const std::string port = av[1];
    const std::string password = av[2];
    if (!checkPort(port)) {
        std::cerr << "Error: Incorrect port" << std::endl;
        return 1;
    }
    if (!checkPass(password)) {
        std::cerr << "Error: Incorrect password" << std::endl;
        return 1;
    }
    try {
        Server s(port, password);
        s.run();
    }
    catch(const std::exception& e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

bool checkPort(const std::string &str)
{
    if (str.empty() || str.size() > 5)
        return false;
    for (size_t i = 0; i < str.size(); ++i) {
        if (!std::isdigit(str[i]))
            return false;
    }
    int port = atoi(str.c_str());
    if (port < 1024 || port > 65535)
        return false;
    return true;
}

bool checkPass(const std::string &password)
{
    if (password.empty() || password.size() > 10)
        return false;
    for (size_t i = 0; i < password.size(); ++i) {
        if (password[i] == ' ' || password[i] == '\0' || password[i] == '\r' ||
            password[i] == '\n' || password[i] == '\x07')
            return false;
    }
    return true;
}

// int main()
// {
//     Parser parser;

//     std::vector<std::string> lines;
//     lines.push_back(":nick!user@host JOIN #channel");
//     lines.push_back("KICK #chan user1 :reason text");
//     lines.push_back("PRIVMSG #chan :Hello world");

//     lines.push_back("JOIN #chan,#chan2,#chan3 :Hello world");
//     lines.push_back("MADE #chan +k PASSWORD :Hello world");
//     lines.push_back("");

//     for (size_t z = 0; z < lines.size(); z++)
//     {
//         std::string line = lines[z];
//         std::cout << "----------: " << line << std::endl;

//         Command c = parser.parse(line);
//         std::cout << "Prefix: " << c.getPrefix() << std::endl;
//         std::cout << "Command: " << c.getCommand() << std::endl;
//         for (size_t i = 0; i < c.getParams().size(); i++) std::cout << i << ": " << c.getParams()[i] << std::endl;
//         std::cout << "Text: " << c.getText() << std::endl;
//     }

//     return 0;
// }