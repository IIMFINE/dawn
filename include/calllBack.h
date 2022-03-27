#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 #include <unistd.h>
#include "type.h"

typedef std::function<void()>  callback_t;
typedef std::function<callback_t()> returnCallback_t;

#endif