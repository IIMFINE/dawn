#ifndef _TYPE_H_
#define _TYPE_H_
#include <functional>


#define FALSE   0
#define TRUE    1

#define PROCESS_FAIL    0
#define PROCESS_SUCCESS 1
#define INVALID_VALUE  -1

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint64_t;

#define LINUX_ERROR_PRINT() printf("%s\n",strerror(errno));

template<typename T>
class singleton
{
public:
    ~singleton();
    singleton(const singleton&) = delete;
    singleton& operator=(const singleton&) = delete;
    static T* getInstance()
    {
        if(instance == nullptr)
        {
            instance = new T();
        }
        return instance;
    }

private:
    static T* instance;
};

template<typename T>
T* singleton<T>::instance = nullptr;

#endif