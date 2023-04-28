#include "gtest/gtest.h"
#include "test_helper.h"

namespace dawn
{
  std::map<std::string, unsigned int> timeCounter::timeSpan_{};
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
