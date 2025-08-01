/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_SOCKS5_CLIENT_SOCKET_DO_LOOP \
  case STATE_AUTH:                         \
    rv = DoAuth(rv);                       \
    break;

// Set authentication method.
#define BRAVE_SOCKS5_CLIENT_SOCKET_DO_GREET_WRITE     \
  static_assert(kSOCKS5GreetWriteData.size() == 3u);  \
  std::string greeting(kSOCKS5GreetWriteData.begin(), \
                       kSOCKS5GreetWriteData.end());  \
  greeting.back() = static_cast<char>(auth_method()); \
  auto greet_buffer = base::MakeRefCounted<StringIOBuffer>(std::move(greeting));

#define BRAVE_SOCKS5_CLIENT_SOCKET_DO_GREET_READ_COMPLETE_1 \
  }                                                         \
  if (read_data[1] != auth_method()) {
#define BRAVE_SOCKS5_CLIENT_SOCKET_DO_GREET_READ_COMPLETE_2 \
  next_state_ = STATE_AUTH;

#include "base/check.h"
#include "base/check_op.h"
#include "base/notreached.h"

#include <net/socket/socks5_client_socket.cc>
#undef BRAVE_SOCKS5_CLIENT_SOCKET_DO_GREET_READ_COMPLETE_2
#undef BRAVE_SOCKS5_CLIENT_SOCKET_DO_GREET_READ_COMPLETE_1
#undef BRAVE_SOCKS5_CLIENT_SOCKET_DO_GREET_WRITE
#undef BRAVE_SOCKS5_CLIENT_SOCKET_DO_LOOP

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "base/compiler_specific.h"
#include "net/base/io_buffer.h"
#include "net/socket/socks5_client_socket.h"

namespace net {

namespace {

constexpr size_t kSOCKSAuthUsernamePasswordResponseLen = 2;

HostPortPair ToLegacyDestinationEndpoint(
    const TransportSocketParams::Endpoint& endpoint) {
  if (std::holds_alternative<url::SchemeHostPort>(endpoint)) {
    return HostPortPair::FromSchemeHostPort(
        std::get<url::SchemeHostPort>(endpoint));
  }

  DCHECK(std::holds_alternative<HostPortPair>(endpoint));
  return std::get<HostPortPair>(endpoint);
}

}  // namespace

int SOCKS5ClientSocket::DoAuth(int rv) {
  rv = Authenticate(rv, net_log_, io_callback_);
  next_state_ = (rv == OK ? STATE_HANDSHAKE_WRITE : STATE_AUTH);
  return rv;
}

uint8_t SOCKS5ClientSocket::auth_method() {
  return 0x00;
}

int SOCKS5ClientSocket::Authenticate(int rv,
                                     NetLogWithSource& net_log,
                                     CompletionRepeatingCallback& callback) {
  DCHECK_EQ(OK, rv);
  return OK;
}

SOCKS5ClientSocketAuth::SOCKS5ClientSocketAuth(
    std::unique_ptr<StreamSocket> transport_socket,
    const HostPortPair& destination,
    const NetworkTrafficAnnotationTag& traffic_annotation,
    const TransportSocketParams::Endpoint& proxy_endpoint)
    : SOCKS5ClientSocket(std::move(transport_socket),
                         destination,
                         traffic_annotation),
      proxy_host_port_(ToLegacyDestinationEndpoint(proxy_endpoint)),
      next_state_(STATE_INIT_WRITE) {}

SOCKS5ClientSocketAuth::~SOCKS5ClientSocketAuth() = default;

const std::string& SOCKS5ClientSocketAuth::username() {
  return proxy_host_port_.username();
}

const std::string& SOCKS5ClientSocketAuth::password() {
  return proxy_host_port_.password();
}

bool SOCKS5ClientSocketAuth::do_auth() {
  return username().size() != 0 || password().size() != 0;
}

uint8_t SOCKS5ClientSocketAuth::auth_method() {
  if (!do_auth())
    return 0x00;
  return 0x02;
}

int SOCKS5ClientSocketAuth::Authenticate(
    int rv,
    NetLogWithSource& net_log,
    CompletionRepeatingCallback& callback) {
  if (!do_auth()) {
    DCHECK_EQ(OK, rv);
    return OK;
  }
  do {
    switch (next_state_) {
      case STATE_INIT_WRITE: {
        DCHECK_EQ(OK, rv);
        // Initialize the buffer with
        //        0x01, usernamelen, username, passwordlen, password
        size_t usernamelen = username().size();
        size_t passwordlen = password().size();
        buffer_ = std::string(1 + 1 + usernamelen + 1 + passwordlen, 0);
        buffer_[0] = 0x01;
        buffer_[1] = usernamelen;
        buffer_.replace(2, usernamelen, username());
        buffer_[2 + usernamelen] = passwordlen;
        buffer_.replace(2 + usernamelen + 1, passwordlen, password());
        DCHECK_EQ(buffer_.size(), 2 + usernamelen + 1 + passwordlen);
        buffer_left_ = buffer_.size();
        next_state_ = STATE_WRITE;
        rv = OK;
        break;
      }
      case STATE_WRITE:
        DCHECK_EQ(OK, rv);
        DCHECK_LT(0u, buffer_left_);
        iobuf_ = base::MakeRefCounted<IOBufferWithSize>(buffer_left_);
        iobuf_->span().copy_from_nonoverlapping(
            base::as_byte_span(buffer_).last(buffer_left_));
        next_state_ = STATE_WRITE_COMPLETE;
        net_log.BeginEvent(NetLogEventType::SOCKS5_AUTH_WRITE);
        rv = transport_socket_->Write(iobuf_.get(), buffer_left_, callback,
                                      traffic_annotation_);
        break;

      case STATE_WRITE_COMPLETE:
        // TODO(riastradh): Zero iobuf?  Zero buffer?
        net_log.EndEventWithNetErrorCode(NetLogEventType::SOCKS5_AUTH_WRITE,
                                         std::max(rv, 0));
        if (rv < 0) {
          next_state_ = STATE_BAD;
          return rv;
        }
        DCHECK_LE(static_cast<size_t>(rv), buffer_left_);
        buffer_left_ -= rv;
        next_state_ = (buffer_left_ == 0 ? STATE_INIT_READ : STATE_WRITE);
        rv = OK;
        break;

      case STATE_INIT_READ:
        DCHECK_EQ(OK, rv);
        buffer_.clear();
        buffer_left_ = kSOCKSAuthUsernamePasswordResponseLen;
        iobuf_ = base::MakeRefCounted<IOBufferWithSize>(buffer_left_);
        next_state_ = STATE_READ;
        rv = OK;
        break;

      case STATE_READ:
        DCHECK_EQ(OK, rv);
        iobuf_ = base::MakeRefCounted<IOBufferWithSize>(buffer_left_);
        next_state_ = STATE_READ_COMPLETE;
        net_log.BeginEvent(NetLogEventType::SOCKS5_AUTH_READ);
        rv = transport_socket_->Read(iobuf_.get(), buffer_left_, callback);
        break;

      case STATE_READ_COMPLETE:
        net_log.EndEventWithNetErrorCode(NetLogEventType::SOCKS5_AUTH_READ,
                                         std::max(rv, 0));
        if (rv < 0) {
          next_state_ = STATE_BAD;
          return rv;
        }
        DCHECK_LE(static_cast<size_t>(rv), buffer_left_);
        buffer_.append(iobuf_->data(), rv);
        buffer_left_ -= rv;
        next_state_ = (buffer_left_ == 0 ? STATE_DONE : STATE_READ);
        rv = OK;
        break;

      case STATE_DONE: {
        DCHECK_EQ(OK, rv);
        DCHECK_EQ(buffer_.size(), kSOCKSAuthUsernamePasswordResponseLen);
        static_assert(kSOCKSAuthUsernamePasswordResponseLen == 2, "bad size");
        uint8_t ver = buffer_[0];
        uint8_t status = buffer_[1];
        next_state_ = STATE_BAD;  // Caller had better stop here.
        if (ver != 0x01 || status != 0x00)
          return ERR_FAILED;
        return OK;
      }

      case STATE_BAD:
      default:
        NOTREACHED() << "bad state";
    }
  } while (rv != ERR_IO_PENDING);
  return rv;
}

}  // namespace net
