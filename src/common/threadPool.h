#ifndef _THREAD_FUNC_H_
#define _THREAD_FUNC_H_

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

#include "funcWrapper.h"
#include "lockFree.h"
#include "setLogger.h"
#include "type.h"

namespace dawn
{
    class threadPool;
    class threadPoolManager;

    class threadPool
    {
    public:
        enum class ENUM_THREAD_STATUS
        {
            STOP,
            RUN,
            EXIT
        };

        struct workQueue_t
        {
            std::mutex queueMutex;
            std::vector<FuncWrapper> workQueue;
        };

        threadPool() = default;
        threadPool(const threadPool &) = delete;
        threadPool &operator=(const threadPool &) = delete;

        ~threadPool();
        void init(uint32_t workThreadNum = 1);
        void addWorkThread(uint32_t workThreadNum = 1);

        void workThreadMaintainer();

        template <typename FUNC_T>
        void eventDriverThread(FUNC_T returnCb)
        {
            while (1)
            {
                if (run_thread_flag_ == ENUM_THREAD_STATUS::RUN)
                {
                    if (pushWorkQueue(returnCb()) != PROCESS_SUCCESS)
                    {
                        LOG_ERROR("push work task to queue wrong");
                    }
                }
                else if (run_thread_flag_ == ENUM_THREAD_STATUS::STOP)
                {
                    std::this_thread::yield();
                }
                else if (run_thread_flag_ == ENUM_THREAD_STATUS::EXIT)
                {
                    LOG_WARN("event thread exit");
                    break;
                }
            }
        }

        template <typename FUNC_T>
        void setEventThread(FUNC_T &&returnCb)
        {
            threadList_.emplace_back(std::make_unique<std::thread>(std::bind(&threadPool::eventDriverThread<FUNC_T>, this, std::forward<FUNC_T>(returnCb))));
        }

        template <typename FUNC_T>
        int pushWorkQueue(FUNC_T &&workTask)
        {
            return pushWorkQueueImpl(std::forward<FUNC_T>(workTask));
        }

        template <typename T, std::enable_if_t<dawn::is_callable<T>::value, int> = 0>
        int pushWorkQueueImpl(T &&workTask)
        {
            auto p_LF_node = new LF_Node<FuncWrapper>(std::forward<T>(workTask));
            taskStack_.pushNode(p_LF_node);
            should_wakeup_.store(true, std::memory_order_release);
            taskNumber_.fetch_add(1, std::memory_order_release);
            threadCond_.notify_one();
            return PROCESS_SUCCESS;
        }

        template <typename T, std::enable_if_t<!dawn::is_callable<T>::value, int> = 0>
        int pushWorkQueueImpl(T &&workTask)
        {
            return PROCESS_SUCCESS;
        }

        void quitAllThreads();
        void haltAllThreads();
        void runAllThreads();

    private:
        bool popWorkQueue(FuncWrapper &taskNode);
        void workThreadRun();

    private:
        volatile ENUM_THREAD_STATUS run_thread_flag_ = ENUM_THREAD_STATUS::STOP;
        std::mutex queueMutex_;
        std::mutex threadListMutex_;
        std::atomic_int taskNumber_ = 0;
        std::atomic_bool should_wakeup_ = false;
        LockFreeStack<FuncWrapper> taskStack_;
        std::condition_variable threadCond_;
        std::vector<std::unique_ptr<std::thread>> threadList_;
    };

    class threadPoolManager
    {
    public:
        threadPoolManager() = default;
        ~threadPoolManager() = default;
        template <typename... executeTask_t>
        std::shared_ptr<threadPool> createThreadPool(int workThreadNum = 1, executeTask_t &&...executeTasks)
        {
            auto spyThreadPool = std::make_shared<threadPool>();
            spyThreadPool->init(workThreadNum);
            if (sizeof...(executeTasks) != 0)
            {
                std::initializer_list<int>{(spyThreadPool->setEventThread(std::forward<executeTask_t>(executeTasks)), 0)...};
            }
            thread_pool_group_.push_back(spyThreadPool);
            return spyThreadPool;
        }
        void threadPoolExecute();
        void threadPoolHalt();
        void threadPoolDestroy();

    private:
        std::vector<std::shared_ptr<threadPool>> thread_pool_group_;
    };

};

#endif
