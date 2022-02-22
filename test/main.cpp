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
    //LOG4CPLUS_DEBUG(DAWN_LOG, "hello world");
    net::memPoolInit();
    auto testThread1 = [](){
        memoryNode_t *tempNode = nullptr;
        int j = 0;
        while(1)
        {
            // for(int i = 1; i < net::MAX_MEM_BLOCK - 10; i++)
            for(int i = 256;;)
            {
                tempNode = net::allocMem(i);
                if(tempNode == nullptr)
                {
                    LOG4CPLUS_DEBUG(DAWN_LOG, "cant alloc mem size is "<<i );
                }
                else
                {
                    // LOG4CPLUS_DEBUG(DAWN_LOG, "alloc successfully mem size is "<<i );
                }
                if(net::freeMem(tempNode) == false)
                {
                    LOG4CPLUS_DEBUG(DAWN_LOG, "cant free mem ");
                }
            }
        }
    };
    // testThread1();
    std::thread a(testThread1);
    std::thread b(testThread1);
    while(1) sleep(1);
    a.join();
    b.join();
    return 0;
#endif
}