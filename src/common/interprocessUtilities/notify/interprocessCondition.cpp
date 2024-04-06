#include "interprocessUtilities/notify/interprocessCondition.h"

#include <errno.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <filesystem>
#include <fstream>

#include "setLogger.h"
#include "type.h"

namespace dawn
{
    namespace fs = std::filesystem;

    InterprocessCondition::~InterprocessCondition()
    {
        if (inotify_fd_ != -1)
        {
            inotify_rm_watch(inotify_fd_, inotify_wd_);
            close(inotify_fd_);
        }
    }

    bool InterprocessCondition::init(std::string_view identity)
    {
        if (identity.empty())
        {
            return PROCESS_FAIL;
        }

        std::string inotify_file_path = PREFIX_INOTIFY_FILE_PATH + std::string(identity);

        inotify_file_path_ = inotify_file_path;

        if (!fs::exists(inotify_file_path))
        {
            std::ofstream ofs(inotify_file_path);
            ofs.close();
        }

        inotify_fd_ = inotify_init();

        if (inotify_fd_ == -1)
        {
            LOG_ERROR("create inotify fd failed, reason: {}", strerror(errno));
            return PROCESS_FAIL;
        }

        inotify_wd_ = inotify_add_watch(inotify_fd_, inotify_file_path.c_str(), IN_MODIFY | IN_OPEN);

        if (inotify_wd_ == -1)
        {
            LOG_ERROR("add inotify watch failed, reason: {}", strerror(errno));
            return PROCESS_FAIL;
        }

        return PROCESS_SUCCESS;
    }

    bool InterprocessCondition::wait()
    {
        uint32_t avail = 0;
        ioctl(inotify_fd_, FIONREAD, &avail);

        if (avail == 0)
        {
            avail = 128;
        }

        char buff[avail];
        auto len = read(inotify_fd_, buff, sizeof(buff));
        if (len == -1)
        {
            LOG_ERROR("read inotify fd failed, reason: {}", strerror(errno));
            return PROCESS_FAIL;
        }
        return PROCESS_SUCCESS;
    }

    bool InterprocessCondition::wait_for(std::chrono::milliseconds timeout_ms)
    {
        FD_ZERO(&select_fds_);
        FD_SET(inotify_fd_, &select_fds_);
        struct timeval timeout;
        timeout.tv_sec = timeout_ms.count() / 1000;
        timeout.tv_usec = (timeout_ms.count() % 1000) * 1000;
        auto ret = select(inotify_fd_ + 1, &select_fds_, nullptr, nullptr, &timeout);

        if (ret < -1)
        {
            LOG_ERROR("select failed, reason: {}", strerror(errno));
            return PROCESS_FAIL;
        }
        else if (ret == 0)
        {
            LOG_ERROR("select timeout");
            return PROCESS_FAIL;
        }

        if (FD_ISSET(inotify_fd_, &select_fds_))
        {
            uint32_t avail = 0;

            ioctl(inotify_fd_, FIONREAD, &avail);

            LOG_DEBUG("inotify fd is ready to read. avail: {}", avail);

            char buff[avail];

            // select trigger method is level trigger, so need to read all events.
            auto len = read(inotify_fd_, buff, sizeof(buff));
            if (len == -1)
            {
                LOG_ERROR("read inotify fd failed, reason: {}", strerror(errno));
                return PROCESS_FAIL;
            }
            return PROCESS_SUCCESS;
        }

        return PROCESS_FAIL;
    }

    bool InterprocessCondition::wait(std::function<bool(void)> condition)
    {
        while (!condition())
        {
            if (wait() == PROCESS_FAIL)
            {
                return PROCESS_FAIL;
            }
        }
        return PROCESS_SUCCESS;
    }

    bool InterprocessCondition::wait_for(std::chrono::milliseconds timeout_ms, std::function<bool(void)> condition)
    {
        auto start = std::chrono::steady_clock::now();
        while (!condition())
        {
            if (wait_for(timeout_ms) == PROCESS_FAIL)
            {
                return PROCESS_FAIL;
            }

            auto end = std::chrono::steady_clock::now();

            if (end > timeout_ms + start)
            {
                return PROCESS_FAIL;
            }
        }
        return PROCESS_SUCCESS;
    }

    bool InterprocessCondition::notify_all()
    {
        std::ofstream ofs(inotify_file_path_, std::ios::app);
        ofs.close();
        return PROCESS_SUCCESS;
    }

}  // namespace dawn
