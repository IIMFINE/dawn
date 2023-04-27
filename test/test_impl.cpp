#include "gtest/gtest.h"
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <future>

#include "test_helper.h"

#include "common/memoryPool.h"
#include "common/multicast.h"
#include "discovery/discovery.h"
#include "common/baseOperator.h"
#include "transport/shmTransport.h"
#include "transport/qosController.h"

TEST(test_dawn, find_bit_position_benchmark)
{
  using namespace dawn;
  using namespace std::chrono;
  unsigned long total_time_span = 0;
  unsigned int loop_time = 0xfffffff;
  for (unsigned int i = 0;i < loop_time;i++)
  {
    {
      auto num = rand() % 0x800;
      timeCounter a("hello3");
      getTopBitPosition(num);
    }
    total_time_span += timeCounter::getTimeSpan("hello3");
  }

  LOG_INFO("loop find bit position spend {} nanosecond", total_time_span / loop_time);

  total_time_span = 0;
  for (unsigned int i = 0;i < loop_time;i++)
  {
    {
      auto num = rand() % 0x800;
      timeCounter a("hello2");
      getTopBitPosition_2(num);
    }
    total_time_span += timeCounter::getTimeSpan("hello2");
  }

  LOG_INFO("judgement find bit position spend {} nanosecond", total_time_span / loop_time);
}


TEST(test_dawn, test_memory_benchmark)
{
  using namespace dawn;
  using namespace std::chrono;
  unsigned long total_time_span = 0;
  int loop_time = 0xfffffff;
  allocMem(1);
  for (int i = 0;i < loop_time;i++)
  {
    {
      timeCounter counter("test1");
      auto test1 = allocMem(50);
      freeMem(test1);
    }
    total_time_span += timeCounter::getTimeSpan("test1");
  }

  LOG_INFO("mempool spend average time {} nanoseconds", total_time_span / loop_time);

  total_time_span = 0;
  for (int i = 0; i < loop_time; i++)
  {
    {
      timeCounter counter("test2");
      auto test1 = malloc(50);
      free(test1);
    }
    total_time_span += timeCounter::getTimeSpan("test2");
  }
  LOG_INFO("malloc spend average time {} nanoseconds", total_time_span / loop_time);
}

TEST(test_dawn, test_TL_memory)
{
  using namespace dawn;
  SET_LOGGER_FILENAME("memory");
  auto future1 = std::async(std::launch::async, []() {
    auto test1 = allocMem(50);
    if (test1 == nullptr)
    {
      LOG_ERROR("alloc a nullptr");
      throw std::runtime_error("dawn: alloc a nullptr");
    }
    });
  auto future2 = std::async(std::launch::async, []() {
    auto test1 = allocMem(50);
    if (test1 == nullptr)
    {
      LOG_ERROR("alloc a nullptr");
      throw std::runtime_error("dawn: alloc a nullptr");
    }
    });
  auto future3 = std::async(std::launch::async, []() {
    auto test1 = allocMem(50);
    if (test1 == nullptr)
    {
      LOG_ERROR("alloc a nullptr");
      throw std::runtime_error("dawn: alloc a nullptr");
    }
    });
  auto future4 = std::async(std::launch::async, []() {
    auto test1 = allocMem(50);
    if (test1 == nullptr)
    {
      LOG_ERROR("alloc a nullptr");
      throw std::runtime_error("dawn: alloc a nullptr");
    }
    });
}

TEST(test_dawn, test_multicast_rx)
{
  using namespace dawn;
  SET_LOGGER_FILENAME("subscription");
  multicast a("224.0.0.2", 2153);
  char data[40];
  auto dead_end = 10;
  while (dead_end--)
  {
    if (a.recvFromSocket(data, sizeof(data)) > 0)
    {
      break;
    }
    LOG_ERROR("don't rx message");
    std::this_thread::sleep_for(std::chrono::seconds(100));
  }
  std::cout << "end" << std::endl;
}

TEST(test_dawn, test_multicast_send)
{
  using namespace dawn;
  SET_LOGGER_FILENAME("publisher");
  multicast a(dawn::INVALID_IP);
  char data[] = "hello world";
  auto data_len = sizeof(data);
  auto dead_end = 1;
  while (dead_end--)
  {
    /* code */
    a.send2Socket(data, data_len, "224.0.0.2:2153");
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }
}

TEST(test_dawn, test_discovery_pub)
{
  SET_LOGGER_FILENAME("discovery_pub");
  dawn::memPoolInit();
  using namespace dawn;
  //argv is multicast_ip topic_name participant_name flag
  //total sum is 5
  LOG_ERROR("hello world");
  LOG_DEBUG("******************");
  auto multicast_ip = "224.0.0.2";
  unsigned short port = 2152;
  auto topic_name = "dawn_discovery";
  auto participant_name = "discovery1";
  auto flag = identityMessageFormat::FLAG_TYPE::WRITE;
  netDiscovery   a(multicast_ip, port, INVALID_IP, topic_name, participant_name, flag);
  a.pronouncePresence("2152");
}

TEST(test_dawn, test_discovery_sub)
{
  SET_LOGGER_FILENAME("discovery_sub");
  dawn::memPoolInit();
  using namespace dawn;
  //argv is multicast_ip topic_name participant_name flag
  //total sum is 5
  LOG_ERROR("hello world");
  LOG_DEBUG("******************");
  auto multicast_ip = "224.0.0.2";
  unsigned short port = 2152;
  auto topic_name = "dawn_discovery";
  auto participant_name = "discovery1";
  auto flag = identityMessageFormat::FLAG_TYPE::READ;
  netDiscovery   a(multicast_ip, port, INVALID_IP, topic_name, participant_name, flag);
  a.discoveryParticipants();
}

TEST(test_dawn, test_hash)
{
  std::string test_str("hello world");
  auto result = std::hash<std::string>{}(test_str);
  std::cout << result << std::endl;
  auto result2 = std::hash<std::string>{}(test_str);
  std::cout << result2 << std::endl;
}

TEST(test_dawn, test_shm_pool)
{
  using namespace dawn;
  shmMsgPool test_pool;
  for (int i = 0; i < 100;i++)
  {
    auto index1 = test_pool.requireOneBlock();
    std::cout << index1 << "  ";
  }
  std::cout << std::endl;
}

#include <signal.h>

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
      if (shm_tp.read(data, len, TP::BLOCK_TYPE::BLOCK) == PROCESS_FAIL)
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

TEST(test_dawn, test_shmTp_read_one_slot)
{
  using namespace dawn;
  using TP = abstractTransport;
  shmTransport shm_tp("hello_world_dawn");
  {
    char* data = new char[9 * 1024 * 1024];
    uint32_t len = 0;
    if (shm_tp.read(data, len, TP::BLOCK_TYPE::NON_BLOCK) == PROCESS_FAIL)
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

  for (int i = 0; i < 0xffff; i++)
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
  for (int i = 0;; i++)
  {
    std::string data = "helloWorld";
    data += std::to_string(i);
    std::cout << "publish data " << data << std::endl;
    if (shm_tp.write(data.c_str(), data.size()) == PROCESS_FAIL)
    {
      std::cout << "failed" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

TEST(test_dawn, test_shmChannel_notify)
{
  using namespace dawn;
  shmChannel a("dawn_test_channel");
  a.notifyAll();
}

TEST(test_dawn, test_shmChannel_wait)
{
  using namespace dawn;
  shmChannel a("dawn_test_channel");
  a.waitNotify();
}

TEST(test_dawn, test_shmChannel_try_wait)
{
  using namespace dawn;
  shmChannel a("dawn_test_channel");
  if (a.tryWaitNotify() == false)
  {
    std::cout << "failed" << std::endl;
  }
}
