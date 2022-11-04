#include <iostream>
#include <type_traits>
#include <vector>
#include <mysql/mysql.h>
#include <string>
#include <tuple>
#include <list>
#include <cstdint>
#include <inttypes.h>
#include <exception>
#include <functional>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <sched.h>
#include "threadPool.h"
#include "type.h"
#include "memoryPool.h"
#include <type_traits>
#include <string.h>
#include <vector>
#include "setLogger.h"
#include "baseOperator.h"
#include "hazardPointer.h"

struct test_mem
{
    test_mem* next;
};



int main()
{
#if 0
    MYSQL *sql = nullptr;
    sql = mysql_init(sql);
    const char* host = "localhost";
    const char* user = "root";
    const char* pwd = "12345678!";
    const char* dbName = "runnood";
    int port = 3306;
    sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
    if(sql == nullptr)
    {
        std::cout<<"sql error"<<std::endl;
    }

    MYSQL_RES *result;
    MYSQL_ROW row;
    int res;
    res = mysql_query(sql, "create table 'studyPlan' ('id' int, 'title' vachar(100))");
    res = mysql_query(sql, "show tables");
    // res = mysql_query(sql, "describe studyPlan");
    if(1)
    {
        result = mysql_use_result(sql);
        for(uint32_t i = 0; i < mysql_field_count(sql); i++)
        {
            row = mysql_fetch_row(result);
            if(row <= 0)
            {
                break;
            }

            for(uint32_t j=0; j < mysql_num_fields(result); ++j)
            {
                std::cout << row[j] << " "<<std::endl;
            }
        }
        mysql_free_result(result);
    }
    else
    {
        std::cout<<"mysql error"<<std::endl;
    }
    return 0;

#endif

#if 1
    net::memPoolInit();
    auto testThread1 = [](){
        memoryNode_t *tempNode = nullptr;
        while(1)
        {
            for(int i = 1; i < 2; i++)
            {
                tempNode = net::allocMem(i);
                if(tempNode == nullptr)
                {
                    LOG_INFO("cant alloc mem size is "<<i );
                    continue;
                }
                else
                {
                    LOG_DEBUG("alloc successfully mem size is "<<i );
                }
                if(net::freeMem(tempNode) == false)
                {
                    LOG_INFO("cant free mem ");
                }
            }
        }
    };
    for(int i = 0; i< 2; i++)
    {
        std::thread a(testThread1);
        a.detach();
    }

    // while(1) sleep(1);
    sleep(5);
    return 0;
#endif

#if 0
    std::atomic<test_mem*> head(new test_mem);
    head.load()->next = new test_mem;
    head.load()->next->next = nullptr;
    auto a = [&](){
        //pop item
        int64_t times = 0;
        while (1)
        {
            /* code */
            auto temp_node = head.load();
            while(1)
            {
                temp_node = head.load();
                if(temp_node != nullptr)
                {
                    if(head.compare_exchange_weak(temp_node, temp_node->next))
                    {
                        times++;
                        break;
                    }
                    else
                    {
                        LOG_DEBUG("alloc failed");
                        continue;
                    }
                }
            }
            if(times%10000)
            {
                LOG_WARN("alloc");
            }
            //push item
            while (1)
            {
                /* code */
                temp_node->next = head.load();
                if(head.compare_exchange_weak(temp_node->next, temp_node))
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
        }
    };
    for(int i = 0; i < 10; i++)
    {
        std::thread(a).detach();
    }
    while (1)
    {
        /* code */
        std::this_thread::yield();
    }
    
#endif

#if 0
    std::atomic<int> a(0);
    auto b = [&](){
        for(int i = 0; i < 100; i++)
        {
            // a.fetch_add(1, std::memory_order_relaxed);
            int c = a.load(std::memory_order_relaxed);
            do
            {
                /* code */
                c = a.load(std::memory_order_relaxed);
            } while (!a.compare_exchange_weak(c, c + 1, std::memory_order_release, std::memory_order_relaxed));
            
        }
    };
    for(int i = 0; i < 15; i++)
    {
        std::thread(b).detach();
    }
    sleep(5);
    LOG_ERROR("pan a "<<a.load());
#endif
}