#include "gtest/gtest.h"
#include "test_helper.h"

namespace dawn
{
  std::map<std::string, unsigned int> timeCounter::timeSpan_{};
  std::map<std::string, uint64_t> cyclesCounter::timeSpan_{};
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
