//In order to prevent linker error, we should place all the base struct type
//in this head file.By the way,
//we make sure this head file is the lowest level head file.

#ifndef _TYPE_H_
#define _TYPE_H_
#include <functional>
#include <type_traits>

#define FALSE   0
#define TRUE    1

#define PROCESS_FAIL    0
#define PROCESS_SUCCESS 1
#define INVALID_VALUE  -1

#define LINUX_ERROR_PRINT() printf("%s\n",strerror(errno));

template<typename T>
class singleton
{
public:
    ~singleton();
    singleton(singleton&) = delete;
    singleton(const singleton&) = delete;
    singleton& operator=(const singleton&) = delete;
    static T* getInstance()
    {
        static T instance;
        return &instance;
    }
};

namespace special_type_traitor
{

template<typename , typename = void>
struct is_callable_helper : std::false_type{};

template<typename T>
struct is_callable_helper<T, std::void_t<decltype(std::declval<T>()())>> : std::true_type{};

template<typename T>
struct is_callable: is_callable_helper<std::remove_reference_t<T>>{};

};

#endif