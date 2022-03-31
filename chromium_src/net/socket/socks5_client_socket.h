/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_SOCKET_SOCKS5_CLIENT_SOCKET_H_
#define BRAVE_CHROMIUM_SRC_NET_SOCKET_SOCKS5_CLIENT_SOCKET_H_

#include "src/net/socket/socks5_client_socket.h"

#include <memory>
#include <string>

#include "net/socket/transport_connect_job.h"

namespace net {

class NET_EXPORT_PRIVATE SOCKS5ClientSocketAuth : public SOCKS5ClientSocket {
 public:
  SOCKS5ClientSocketAuth(std::unique_ptr<StreamSocket> transport_socket,
                         const HostPortPair& destination,
                         const NetworkTrafficAnnotationTag& traffic_annotation,
                         const TransportSocketParams::Endpoint& proxy_endpoint);
  SOCKS5ClientSocketAuth(const SOCKS5ClientSocketAuth&) = delete;
  SOCKS5ClientSocketAuth& operator=(const SOCKS5ClientSocketAuth&) = delete;
  ~SOCKS5ClientSocketAuth() override;

 private:
  bool do_auth();
  const std::string& username();
  const std::string& password();
  uint8_t auth_method() override;
  int Authenticate(int rv,
                   NetLogWithSource& net_log,
                   CompletionRepeatingCallback& callback) override;
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
};

}  // namespace net

#endif  // BRAVE_CHROMIUM_SRC_NET_SOCKET_SOCKS5_CLIENT_SOCKET_H_
