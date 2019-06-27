#ifndef BRAVE_NET_BASE_HOST_PORT_PAIR_H_
#define BRAVE_NET_BASE_HOST_PORT_PAIR_H_

// Nudges HostPortPair past Chromium's style
// threshold for in-line constructors and destructors.
#define BRAVE_HOST_PORT_PAIR_H_ \
  HostPortPair(const std::string& username, const std::string& password, \
               const std::string& in_host, uint16_t in_port); \
  ~HostPortPair(); \
  HostPortPair(const HostPortPair&); \
  const std::string& username() const; \
  const std::string& password() const; \
  void set_username(const std::string& username); \
  void set_password(const std::string& password); \
  bool operator<(const HostPortPair& other) const; \
  bool Equals(const HostPortPair& other) const; \
 private: \
  std::string username_; \
  std::string password_;

#include "../../../../net/base/host_port_pair.h"

#undef BRAVE_HOST_PORT_PAIR_H_

#endif  // BRAVE_NET_BASE_HOST_PORT_PAIR_H_
