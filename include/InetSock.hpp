#pragma once

#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/uio.h>

#include "InetAddr.hpp"
#include "Endian.hpp"

namespace netio {

#define INET_PORT_CAST(port) static_cast<uint16_t>(port)
#define INET_FD_CAST(fd)  static_cast<int>(fd)

class InetSock {
 public:
  InetSock(int fd);
  virtual ~InetSock();
  
  void enableReuseAddr(bool enable);
  void enableReusePort(bool enable);

  void setNonblocking(bool enable);

  int setRecvBufSize(int size);
  int setSendBufSize(int size);

  int setSendTimeout(int msec);
  int setRecvTimeout(int msec);

  int getSocketError() const;

  InetAddr getLocalAddr() const;
  InetAddr getPeerAddr() const;


  ssize_t sendmsg(const struct msghdr& msg, int flags = 0) {
    return ::sendmsg(_fd, &msg, flags);
  }
  
  ssize_t recvmsg(struct msghdr& msg, int flags = 0) {
    return ::recvmsg(_fd, &msg, flags);
  }

  int bind(const InetAddr& addr);
  int bind(const struct sockaddr_in& addr);
  int bind(uint16_t port);

  int getFd() const {
    return _fd;
  }

  void close() {
    ::close(_fd);
    _fd = -1;
  }

 protected:
  int _fd;
};


class StreamSocket : public InetSock {
 public:
  /**
   * Usually for tpc accept for initial with socket fd.
   */
  StreamSocket(int fd);

  /**
   * For server socket or client socket.
   */
  explicit StreamSocket(uint16_t port);

  /**
   * For server socket or client socket.
   */
  StreamSocket(const struct sockaddr_in& sockaddr);

  
  int setKeepAlive(bool enable);
  
  ssize_t send(const void* buf, size_t len, int flags = 0) {
    return ::send(_fd, buf, len, flags);
  }
  
  ssize_t recv(void* buf, size_t len, int flags = 0) {
    return ::recv(_fd, buf, len, flags);
  }

  ssize_t writev(const struct iovec* iov, int iovcnt) {
    return ::writev(_fd, iov, iovcnt);
  }
  
  ssize_t readv(const struct iovec* iov, int iovcnt) {
    return ::readv(_fd, iov, iovcnt);
  }
};

/**
 * Tcp client socket
 */
class Socket : public StreamSocket {
 public:
  Socket(int fd) : StreamSocket(fd) {};
  explicit Socket(uint16_t port) : StreamSocket(port) {};
  Socket(const struct sockaddr_in& sockaddr) : StreamSocket(sockaddr) {};

  
  int connect(const struct sockaddr_in& remote);
};

/**
 * Tcp Server socket 
 */
class ServerSocket : public StreamSocket {
 public:
  ServerSocket(int fd) : StreamSocket(fd) {};
  explicit ServerSocket(uint16_t port) : StreamSocket(port) {};
  ServerSocket(const struct sockaddr_in& sockaddr) : StreamSocket(sockaddr) {};
  
  int listen(int backlog = SOMAXCONN);
  int accept(const struct sockaddr_in& clientaddr);
};

/**
 * Udp socket
 */
class DGramSocket : public InetSock {
 public:
  DGramSocket(uint16_t port);
  DGramSocket(const struct sockaddr_in& addr);

  ssize_t recvfrom(void* buf, size_t len, struct sockaddr_in& addr, int flags) {
    socklen_t addrlen = sizeof(addr);
    return ::recvfrom(_fd, buf, len, flags, SOCKADDR_CAST(&addr), &addrlen);
  }

  ssize_t sendto(const void* buf, size_t len, int flags, const InetAddr&  addr) {
    return sendto(buf, len, flags, addr.getSockAddr());
  }

  ssize_t sendto(const void* buf, size_t len, int flags, uint32_t rip, uint16_t rport) {
    struct sockaddr_in addr = {0};
    addr.sin_addr.s_addr = Endian::hton32(rip);
    addr.sin_port = Endian::hton16(rport);
    addr.sin_family = AF_INET;

    return this->sendto(buf, len, flags, addr);
  }
  
  ssize_t sendto(const void* buf, size_t len, int flags, const struct sockaddr_in& addr) {
    return ::sendto(_fd, buf, len, flags, SOCKADDR_CAST(&addr), static_cast<socklen_t>(sizeof(addr)));
  }
};

}

