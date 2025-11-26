#include "Server.hpp"

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

