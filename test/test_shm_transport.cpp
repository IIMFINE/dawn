#include "gtest/gtest.h"
#include <string>
#include <array>
#include <thread>
#include <chrono>
#include <future>
#include "transport/shmTransportController.h"
#include "common/setLogger.h"


TEST(test_dawn, test_shmTp_read_loop_block)
{
  using namespace dawn;
  using TP = abstractTransport;
  shmTransport shm_tp("hello_world_dawn");
  try
  {
    for (;;)
    {
      char* data = new char[9 * 1024 * 1024];
      uint32_t len = 0;
      if (shm_tp.read(data, len, TP::BLOCKING_TYPE::BLOCK) == PROCESS_FAIL)
      {
        std::cout << "failed" << std::endl;
      }
      else
      {
        LOG_INFO("receive data {}", data);
        std::cout << "receive data " << data << std::endl;
        std::cout << "receive data len " << len << std::endl << std::endl;
      }
      delete data;
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    raise(SIGABRT);
  }

}

TEST(test_dawn, test_shmTp_read_one_slot_non_block)
{
  using namespace dawn;
  using TP = abstractTransport;
  shmTransport shm_tp("hello_world_dawn");
  {
    char* data = new char[9 * 1024 * 1024];
    uint32_t len = 0;
    if (shm_tp.read(data, len, TP::BLOCKING_TYPE::NON_BLOCK) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    else
    {
      LOG_INFO("receive data {}", data);
      std::cout << "receive data " << data << std::endl;
      std::cout << "len " << len << std::endl;
    }
    delete data;
  }
}

TEST(test_dawn, test_shmTp_read_one_slot_block)
{
  using namespace dawn;
  using TP = abstractTransport;
  shmTransport shm_tp("hello_world_dawn");
  {
    char* data = new char[9 * 1024 * 1024];
    uint32_t len = 0;
    if (shm_tp.read(data, len, TP::BLOCKING_TYPE::BLOCK) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    else
    {
      LOG_INFO("receive data {}", data);
      std::cout << "receive data " << data << std::endl;
      std::cout << "len " << len << std::endl;
    }
    delete data;
  }
}

TEST(test_dawn, test_shmTp_read_one_slot_block_multi_thread)
{
  using namespace dawn;
  using TP = abstractTransport;
  shmTransport shm_tp("hello_world_dawn");
  auto func = [&]() {
    while (1)
    {
      char* data = new char[9 * 1024 * 1024];
      uint32_t len = 0;
      if (shm_tp.read(data, len, TP::BLOCKING_TYPE::BLOCK) == PROCESS_FAIL)
      {
        std::cout << "failed" << std::endl;
      }
      else
      {
        LOG_INFO("receive data {}", data);
        std::cout << "receive data " << data << std::endl;
        std::cout << "len " << len << std::endl;
      }
      delete data;
    }
  };
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  while (1)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

TEST(test_dawn, test_shmTp_write_small_data)
{
  using namespace dawn;
  shmTransport shm_tp("hello_world_dawn");
  auto i = 0;
  {
    std::string data = "helloWorld";
    data += std::to_string(i);
    if (shm_tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
  }
}


/// @brief publish a 64k bytes data
/// @param  
/// @param  
TEST(test_dawn, test_shmTp_write_large_data)
{
  using namespace dawn;
  SET_LOGGER_FILENAME("write_large_data");
  shmTransport shm_tp("hello_world_dawn");
  {
    std::string data = "helloWorld large data";

    data.resize(64 * 1024);

    std::cout << "publish data " << data << std::endl;
    if (shm_tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

TEST(test_dawn, test_shmTp_write_large_data_loop)
{
  using namespace dawn;
  SET_LOGGER_FILENAME("publisher");
  shmTransport shm_tp("hello_world_dawn");

  for (int i = 0; i < 0xfff; i++)
  {
    std::string data = "helloWorld large data";
    data += std::to_string(i);
    std::cout << "publish data " << data << std::endl;
    data.resize(64 * 1024);
    if (shm_tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    LOG_INFO("publish data i {}", i);
  }
}

TEST(test_dawn, test_shmTp_write_small_data_loop)
{
  using namespace dawn;
  shmTransport shm_tp("hello_world_dawn");
  for (int i = 0;i < 0xf; i++)
  {
    std::string data = "helloWorld";
    data += std::to_string(i);
    std::cout << "publish data " << data << std::endl;
    if (shm_tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
  }
}


TEST(test_dawn, test_shmTp_write_small_data_loop_multithread)
{
  using namespace dawn;
  shmTransport shm_tp("hello_world_dawn");
  std::atomic<uint32_t>   index = 0;
  uint32_t   loop_time = 0xffff;
  auto func = [&]() {
    for (uint32_t i = 0;i < loop_time; i++)
    {
      std::string data = "helloWorld";
      data += std::to_string(index.load());
      index++;
      std::cout << "publish data " << data << std::endl;
      if (shm_tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
      {
        std::cout << "failed" << std::endl;
      }
    }
  };

  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();

  while (1)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

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
    uint32_t count = 0xfff;
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
  while (i--)
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


TEST(test_dawn, shmTpReliableWriteLoopMultiThreads)
{
  using namespace dawn;
  shmTransport tp("hello_dawn", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  uint32_t count = 0xfffff;
  std::atomic<uint32_t> index(0);
  std::atomic<uint32_t> threadNum(4);
  auto func = [&]() {
    for (uint32_t i = 0; i < count;i++)
    {
      std::string data = "hello world";
      data = "hello world" + std::to_string(index.load());
      index++;
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
    threadNum--;
  };

  for (int i = 0;i < 4;i++)
  {
    std::thread(func).detach();
  }
  int i = 1;
  while (i)
  {
    /* code */
    if (threadNum.load() == 0)
    {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

TEST(test_dawn, test_shmTp_read_write_loopback)
{
  using namespace dawn;
  shmTransport shm_tp1("dawn_pipe1", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  shmTransport shm_tp2("dawn_pipe2", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  for (int i = 0;; i++)
  {
    char* data2 = new char[9 * 1024];
    uint32_t len = 0;
    std::cout << "read**********" << std::endl;
    if (shm_tp1.read(data2, len) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    else
    {
      std::cout << "*************read data " << data2 << std::endl;
      std::cout << "write**********" << std::endl;
      //check data
      if (shm_tp2.write(data2, len) == PROCESS_FAIL)
      {
        std::cout << "failed" << std::endl;
      }
      std::cout << "write data " << data2 << std::endl;
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

TEST(test_dawn, test_shmTp_write_read_loopback)
{
  using namespace dawn;
  shmTransport shm_tp1("dawn_pipe1", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  shmTransport shm_tp2("dawn_pipe2", std::make_shared<qosCfg>(qosCfg::QOS_TYPE::RELIABLE));
  for (int i = 0;; i++)
  {
    std::string data = "helloWorld";
    data += std::to_string(i);
    std::cout << "publish data " << data << std::endl;
    if (shm_tp1.write(data.c_str(), data.size()) == PROCESS_FAIL)
    {
      std::cout << "write failed" << std::endl;
      break;
    }

    char* data2 = new char[9 * 1024];
    uint32_t len = 0;
    std::cout << "read" << std::endl;
    if (shm_tp2.read(data2, len) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    else
    {
      std::cout << "****************" << std::endl;
      std::cout << "write data " << data << std::endl;
      std::cout << "read data " << data2 << std::endl;
      std::cout << "****************" << std::endl;
      //check data
      if (len != data.size() || data != data2)
      {
        {
          std::cout << "failed" << std::endl;
          std::cout << "write data " << data << std::endl;
          std::cout << "read data " << data2 << std::endl;
          break;
        }
      }
      std::cout << "receive loopback data " << data2 << std::endl;
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

