#include "gtest/gtest.h"
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <future>

#include "test_helper.h"

#include "common/memoryPool.h"
#include "common/multicast.h"
#include "discovery/discovery.h"
#include "transport/shmTransport.h"
#include "transport/qosController.h"


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

TEST(test_dawn, test_shmChannel_multiThread)
{
  using namespace dawn;
  shmChannel a("dawn_test_channel");
  auto func = [&]() {
    a.waitNotify();
    std::cout << "recevie notify" << std::endl;
  };

  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  std::thread(func).detach();
  while (1)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
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


