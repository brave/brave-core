/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/socket/socks5_client_socket.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/containers/span.h"
#include "base/memory/raw_ptr.h"
#include "net/base/address_list.h"
#include "net/base/test_completion_callback.h"
#include "net/base/winsock_init.h"
#include "net/socket/socket_test_util.h"
#include "net/socket/tcp_client_socket.h"
#include "net/socket/transport_connect_job.h"
#include "net/test/gtest_util.h"
#include "net/test/test_with_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using net::test::IsError;
using net::test::IsOk;

namespace net {
namespace {

class BraveSocks5ClientSocketAuthTest : public PlatformTest,
                                        public WithTaskEnvironment {
 public:
  void SetUp() override {
    PlatformTest::SetUp();
    address_list_ =
        AddressList::CreateFromIPAddress(IPAddress::IPv4Localhost(), 1080);
  }

 protected:
  std::unique_ptr<SOCKS5ClientSocketAuth> BuildSocket(
      base::span<const MockRead> reads,
      base::span<const MockWrite> writes,
      const HostPortPair& proxy_endpoint) {
    data_ = std::make_unique<StaticSocketDataProvider>(reads, writes);
    auto transport = std::make_unique<MockTCPClientSocket>(
        address_list_, nullptr, data_.get());
    transport_socket_ = transport.get();

    TestCompletionCallback connect_callback;
    int rv = transport_socket_->Connect(connect_callback.callback());
    EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
    rv = connect_callback.WaitForResult();
    EXPECT_THAT(rv, IsOk());

    TransportSocketParams::Endpoint endpoint = proxy_endpoint;
    return std::make_unique<SOCKS5ClientSocketAuth>(
        std::move(transport), HostPortPair("example.com", 80),
        TRAFFIC_ANNOTATION_FOR_TESTS, endpoint);
  }

  AddressList address_list_;
  raw_ptr<StreamSocket> transport_socket_ = nullptr;
  std::unique_ptr<SocketDataProvider> data_;
};

TEST_F(BraveSocks5ClientSocketAuthTest, AuthenticatesWithUsernameAndPassword) {
  constexpr uint8_t kGreetingRequest[] = {0x05, 0x01, 0x02};
  constexpr uint8_t kGreetingResponse[] = {0x05, 0x02};
  constexpr uint8_t kAuthRequest[] = {
      0x01, 0x05, 'a', 'l', 'i', 'c', 'e', 0x06, 's', 'e', 'c', 'r', 'e', 't'};
  constexpr uint8_t kAuthResponse[] = {0x01, 0x00};
  constexpr uint8_t kConnectRequest[] = {
      0x05, 0x01, 0x00, 0x03, 0x0b, 'e', 'x', 'a', 'm',
      'p',  'l',  'e',  '.',  'c',  'o',  'm',  0x00, 0x50};
  constexpr uint8_t kConnectResponse[] = {
      0x05, 0x00, 0x00, 0x01, 127, 0, 0, 1, 0x01, 0xbb};

  MockWrite writes[] = {
      MockWrite(ASYNC, base::span(kGreetingRequest)),
      MockWrite(ASYNC, base::span(kAuthRequest)),
      MockWrite(ASYNC, base::span(kConnectRequest)),
  };
  MockRead reads[] = {
      MockRead(ASYNC, base::span(kGreetingResponse)),
      MockRead(ASYNC, base::span(kAuthResponse)),
      MockRead(ASYNC, base::span(kConnectResponse)),
  };

  auto socket = BuildSocket(
      reads, writes, HostPortPair("alice", "secret", "proxy.test", 1080));
  TestCompletionCallback callback;
  int rv = socket->Connect(callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  rv = callback.WaitForResult();
  EXPECT_THAT(rv, IsOk());
  EXPECT_TRUE(socket->IsConnected());
}

TEST_F(BraveSocks5ClientSocketAuthTest, RejectsOversizedCredentials) {
  constexpr uint8_t kGreetingRequest[] = {0x05, 0x01, 0x02};
  constexpr uint8_t kGreetingResponse[] = {0x05, 0x02};

  MockWrite writes[] = {MockWrite(ASYNC, base::span(kGreetingRequest))};
  MockRead reads[] = {MockRead(ASYNC, base::span(kGreetingResponse))};

  std::string long_username(256, 'u');
  auto socket = BuildSocket(
      reads, writes,
      HostPortPair(long_username, "secret", "proxy.test", 1080));

  TestCompletionCallback callback;
  int rv = socket->Connect(callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  rv = callback.WaitForResult();
  EXPECT_THAT(rv, IsError(ERR_SOCKS_CONNECTION_FAILED));
  EXPECT_FALSE(socket->IsConnected());
}

TEST_F(BraveSocks5ClientSocketAuthTest, RejectsAuthenticationFailure) {
  constexpr uint8_t kGreetingRequest[] = {0x05, 0x01, 0x02};
  constexpr uint8_t kGreetingResponse[] = {0x05, 0x02};
  constexpr uint8_t kAuthRequest[] = {
      0x01, 0x05, 'a', 'l', 'i', 'c', 'e', 0x06, 's', 'e', 'c', 'r', 'e', 't'};
  constexpr uint8_t kAuthRejected[] = {0x01, 0x01};

  MockWrite writes[] = {
      MockWrite(ASYNC, base::span(kGreetingRequest)),
      MockWrite(ASYNC, base::span(kAuthRequest)),
  };
  MockRead reads[] = {
      MockRead(ASYNC, base::span(kGreetingResponse)),
      MockRead(ASYNC, base::span(kAuthRejected)),
  };

  auto socket = BuildSocket(
      reads, writes, HostPortPair("alice", "secret", "proxy.test", 1080));
  TestCompletionCallback callback;
  int rv = socket->Connect(callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  rv = callback.WaitForResult();
  EXPECT_THAT(rv, IsError(ERR_SOCKS_CONNECTION_FAILED));
  EXPECT_FALSE(socket->IsConnected());
}

}  // namespace
}  // namespace
