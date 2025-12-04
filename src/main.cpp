#include "../includes/Server.hpp"

int main(int ac, char *av[])
{
    if (ac != 3)
    {
        std::cerr << "Error: executable example: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    std::string port = av[1];
    std::string password = av[2];

    try
    {
        Server s(port, password);
        s.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
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