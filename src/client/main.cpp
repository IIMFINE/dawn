#include <iostream>
#include "type.h"
#include "net.h"
#include <unistd.h>

int main()
{
    net::udpSocket udpClass;
    std::string bindIp("127.0.0.100:2152");
    udpClass.bindSocket(bindIp);
    while(1)
    {
        sleep(1);
    }
}