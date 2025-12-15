#include "./Bot.hpp"

int main(int ac, char *av[])
{
    if (ac != 4)
    {
        std::cerr << "Error: executable example: ./irc_bot <ip> <port> <password>" << std::endl;
        return 1;
    }

    std::string ip = av[1];
    std::string port = av[2];
    std::string password = av[3];

    try
    {
        Bot b(ip, port, password);
        b.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "irc bot error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}