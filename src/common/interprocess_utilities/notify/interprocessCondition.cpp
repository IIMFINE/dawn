#include "interprocess_utilities/notify/interprocessCondition.h"
#include "type.h"
#include "setLogger.h"

#include <filesystem>
#include <fstream>

#include <sys/inotify.h>
#include <errno.h>

namespace dawn
{
  namespace fs = std::filesystem;

  bool InterprocessCondition::init(std::string_view identity)
  {
    if (identity.empty())
    {
      return PROCESS_FAIL;
    }

    std::string inotify_file_path = PREFIX_INOTIFY_FILE_PATH + std::string(identity);

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

    return PROCESS_SUCCESS;
  }

} //namespace dawn
