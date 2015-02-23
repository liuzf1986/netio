
#include <iostream>
#include <vector>
#include <memory>
#include <stdint.h>
#include "InetAddr.hpp"
#include "SingleCache.hpp"
#include "Logger.hpp"
#include "VecBuffer.hpp"
#include "Netpack.hpp"
#include "Channel.hpp"
//#include "TcpConnection.hpp"
//#include "TcpPump.hpp"

#include "MultiplexLooper.hpp"
#include "TcpAcceptor.hpp"

using namespace std;
using namespace netio;

void test_logger(int times) {
  Logger<true> mLogger("/home/liuzf/workspace", "123");
  mLogger.printLogLn(LEVEL_INFO, "hello", "world %llu", 0x01L);

  for (int i = 0; i < times; i ++) {
    LOGI("ttkk", "current number is %d", i);
  }
}

void test_inetaddr(const char* host) {
  InetAddr addr(16);
  std::cout << "addr : " << addr.strIpPort() << std::endl;

  InetAddr addr2(0);

  bool resolved = InetAddr::resolve(host, addr2);

  std::cout << "baidu resolved " << resolved << ", addr= " << addr2.strIp() << std::endl;
}

void test_channelbuffer() {
}

/*
template <class NP>
void test_recvToChannel(Channel<NP>& channel) {
}
*/

void test_channel() {
  /*
  PeerMessage pm;
  Channel<FieldLenNetpack<GenericLenFieldHeader> > channel;
  channel.sendPeerMessage(pm);
  channel.markSended(1000);
  */
}


void test_connection() {
  /*
  TcpConnection<> conn(INET_PORT_CAST(1234));
  std::cout << "fd = " << conn.getFd() << std::endl;
  std::cout << "peer = " << conn.getPeerAddr().strIpPort() << std::endl;
  std::cout << "local = " << conn.getLocalAddr().strIpPort() << std::endl;

  conn.sendInternal();
  */
}


void test_tcppump() {
  /*
  TcpPump pump(INET_PORT_CAST(1233));
  pump.startWork();
  */
}

int main(int argc, char *argv[])
{
  /*
  test_logger(1);
  test_inetaddr("www.baidu.com");
  test_channelbuffer();
  test_channel();
  test_connection();

  test_tcppump();
  */

  MultiplexLooper looper;
  thread _thread(bind(&MultiplexLooper::startLoop, &looper));

  TcpAcceptor acceptor(&looper, INET_PORT_CAST(3001));
  acceptor.attach();


  sleep(1000);
  
  looper.stopLoop();
  _thread.join();
  return 0;
}
















