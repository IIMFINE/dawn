#include <iostream>
#include "type.h"
#include "net.h"
#include <unistd.h>
#include "threadFunc.h"


int main()
{
    net::udpSocket udpClass;
    std::string bindIp("127.0.0.100:2152");
    udpClass.bindSocket(bindIp);
    net::threadPool threadPoolClass;
    //std::thread testRunPool(&net::threadPool::popWorkQueue, &threadPoolClass);
    threadPoolClass.queueIndexMutex.lock();
    
    //std::cout<<" dead lock there"<<std::endl;
    threadPoolClass.queueIndexMutex.lock();
    std::cout<<"no dead lock"<<std::endl;
    while(1)
    {
        sleep(1);
    }
}