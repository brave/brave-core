// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_NET_SOCKET_SOCKS5_CLIENT_SOCKET_AUTH_H_
#define BRAVE_NET_SOCKET_SOCKS5_CLIENT_SOCKET_AUTH_H_

#include "../../../../net/socket/socks5_client_socket.h"

namespace net {

class NET_EXPORT_PRIVATE SOCKS5ClientSocketAuth : public SOCKS5ClientSocket {
 public:
  SOCKS5ClientSocketAuth(std::unique_ptr<ClientSocketHandle> transport_socket,
                         const HostResolver::RequestInfo& req_info,
                         const NetworkTrafficAnnotationTag& traffic_annotation,
                         const HostPortPair& proxy_host_port);
  ~SOCKS5ClientSocketAuth() override;
 private:
  bool do_auth();
  const std::string& username();
  const std::string& password();
  uint8_t auth_method() override;
  int Authenticate(int rv,
                   ClientSocketHandle& transport, NetLogWithSource& net_log,
                   CompletionCallback& callback) override;
  const HostPortPair proxy_host_port_;
  enum {
    STATE_INIT_WRITE = 0,
    STATE_WRITE,
    STATE_WRITE_COMPLETE,
    STATE_INIT_READ,
    STATE_READ,
    STATE_READ_COMPLETE,
    STATE_DONE,
    STATE_BAD,
  } next_state_;
  scoped_refptr<IOBuffer> iobuf_;
  std::string buffer_;
  size_t buffer_left_;
  DISALLOW_COPY_AND_ASSIGN(SOCKS5ClientSocketAuth);
};

}  // namespace net

#endif  // BRAVE_NET_SOCKET_SOCKS5_CLIENT_SOCKET_AUTH_H_
