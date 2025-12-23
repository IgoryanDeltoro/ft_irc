#include "./Bot.hpp"

bool checkIp(const std::string &);
bool checkPort(const std::string &);
bool checkPass(const std::string &);

int main(int ac, char *av[])
{
    if (ac != 4)
    {
        std::cerr << "Error: executable example: ./ircBot <ip> <port> <password>" << std::endl;
        return 1;
    }

    const std::string ip = av[1];
    const std::string port = av[2];
    const std::string password = av[3];

    if (!checkIp(ip)) {
        std::cerr << "Error: Incorrect IP" << std::endl;
        return 1;
    }
    if (!checkPort(port)) {
        std::cerr << "Error: Incorrect port" << std::endl;
        return 1;
    }
    if (!checkPass(password)) {
        std::cerr << "Error: Incorrect password" << std::endl;
        return 1;
    }
    try {
        Bot b(ip, port, password);
        b.run();
    }
    catch(const std::exception& e) {
        std::cerr << "irc.bot error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

bool checkIp(const std::string &ip) {
    if (ip.empty() || ip.size() < 7 || ip.size() > 15 || *(ip.end() - 1) == '.') 
        return false;
    
    int dots = 0;
    int top_bit_n = 0;
    for (size_t i = 0; i < ip.size(); i++) 
    {
        if (std::isdigit(ip[i]))
        {
            top_bit_n = top_bit_n * 10 + (ip[i] - 48);
        }
        else
        {
            if (ip[i] == '.') 
            {
                if (i == 0 || ip[i - 1] == '.') return false;
                dots++;
            }
            else
                return false;
        }

        if ((i + 1) % 3 == 0)
        {
            if (top_bit_n > 255) return false;
            top_bit_n = 0;
        }
    }
    return (dots == 3);
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