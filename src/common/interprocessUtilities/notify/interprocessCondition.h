#ifndef _INTERPROCESS_CONDITION_H_
#define _INTERPROCESS_CONDITION_H_

#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <string>
#include <string_view>

namespace dawn
{

    constexpr const char* PREFIX_INOTIFY_FILE_PATH = "/dev/shm/";

    /// @brief InterprocessCondition is a class to provide a condition variable for interprocess communication.
    /// It do not use a interprocess lock and condition variable, but use inotify to implement the condition variable.
    /// Benefit: Not cause deadlock.
    struct InterprocessCondition
    {
        InterprocessCondition() = default;
        ~InterprocessCondition();
        bool init(std::string_view identity);
        bool wait();
        bool wait_for(std::chrono::milliseconds timeout_ms);
        bool wait(std::function<bool(void)> condition);
        bool wait_for(std::chrono::milliseconds timeout_ms, std::function<bool(void)> condition);
        bool notify_all();

        std::string inotify_file_path_;
        fd_set select_fds_;
        int max_fd_num_;
        int inotify_fd_;
        int inotify_wd_;
    };

}  // namespace dawn

#endif
