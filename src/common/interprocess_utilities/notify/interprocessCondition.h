#ifndef _INTERPROCESS_CONDITION_H_
#define _INTERPROCESS_CONDITION_H_
#include <string>
#include <string_view>

#include <unistd.h>
#include <sys/types.h>

namespace dawn
{

  constexpr const char* PREFIX_INOTIFY_FILE_PATH = "/dev/shm/";

  struct InterprocessCondition
  {
    InterprocessCondition() = default;
    ~InterprocessCondition() = default;
    // InterprocessCondition(std::string_view identity);
    bool init(std::string_view identity);

    std::string    identity_;
    fd_set         select_fds_;
    int            max_fd_num_;
    int            inotify_fd_;
  };

} //namespace dawn

#endif
