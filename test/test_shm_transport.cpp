#include "gtest/gtest.h"
#include <string>
#include <array>
#include <thread>
#include <chrono>
#include <future>
#include "transport/shmTransportController.h"
#include "common/setLogger.h"

TEST(test_dawn, shmTpReliableRead)
{
  using namespace dawn;
  shmTransport tp("hello_dawn", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  char* data = new char[9 * 1024];
  uint32_t len = 0;
  if (tp.read(data, len, abstractTransport::BLOCKING_TYPE::BLOCK) == PROCESS_FAIL)
  {
    std::cout << "failed" << std::endl;
  }
  else
  {
    LOG_INFO("receive data {}", data);
    std::cout << "receive data " << data << std::endl;
    std::cout << "len " << len << std::endl;
  }
}

TEST(test_dawn, shmTpReliableReadLoop)
{
  using namespace dawn;
  shmTransport tp("hello_dawn", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  std::array<char, 9 * 1024> data;
  uint32_t len = 0;
  uint32_t count = 0xfffff;
  for (uint32_t i = 0; i < count; ++i)
  {
    if (tp.read(data.data(), len, abstractTransport::BLOCKING_TYPE::BLOCK) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    else
    {
      LOG_INFO("receive data {}", data.data());
      std::cout << "receive data " << data.data() << std::endl;
      std::cout << "len " << len << std::endl;
      std::memset(data.data(), 0, data.size());
      // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}

TEST(test_dawn, shmTpReliableReadLoopMultiThread)
{
  using namespace dawn;
  SET_LOGGER_FILENAME("read");
  shmTransport tp("hello_dawn", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  auto func = [&]() {
    std::array<char, 9 * 1024> data;
    uint32_t len = 0;
    uint32_t count = 0xfffff;
    for (uint32_t i = 0; i < count; ++i)
    {
      if (tp.read(data.data(), len, abstractTransport::BLOCKING_TYPE::BLOCK) == PROCESS_FAIL)
      {
        std::cout << "failed" << std::endl;
      }
      else
      {
        LOG_INFO("receive data {}", data.data());
        std::cout << "receive data " << data.data() << std::endl;
        std::memset(data.data(), 0, data.size());
        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
  };

  auto future1 = std::async(std::launch::async, func);
  auto future2 = std::async(std::launch::async, func);
  auto future3 = std::async(std::launch::async, func);
  auto future4 = std::async(std::launch::async, func);
  // auto future5 = std::async(std::launch::async, func);
  int i = 10;
  while (i/* condition */)
  {
    /* code */
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

}

TEST(test_dawn, shmTpReliableWrite)
{
  using namespace dawn;
  shmTransport tp("hello_dawn", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  std::string data = "hello world";
  if (tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
  {
    std::cout << "failed" << std::endl;
  }
  else
  {
    LOG_INFO("send data {}", data);
    std::cout << "send data " << data << std::endl;
  }
}

TEST(test_dawn, shmTpReliableWriteLoop)
{
  using namespace dawn;
  shmTransport tp("hello_dawn", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  std::string data = "hello world";
  uint32_t count = 0xfffffff;
  for (uint32_t i = 0; i < count;i++)
  {
    data = "hello world" + std::to_string(i);
    if (tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    else
    {
      LOG_INFO("send data {}", data);
      // std::cout << "send data " << data << std::endl;
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
