#ifndef _TYPE_H_
#define _TYPE_H_

#define FALSE   0
#define TRUE    1

#define PROCESS_FAIL    0
#define PROCESS_SUCCESS 1

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint64_t;

#define LINUX_ERROR_PRINT() printf("%s\n",strerror(errno));

#endif