#include <iostream>

int main(int ac, char *av[], char *env[])
{
    if (ac !=3)
    {
        std::cerr << "Error: executable example: ./ircserv <port> <password>" << std::endl;
    }

    std::string port = av[1];
    std::string password = av[2];

    return 0;
}

