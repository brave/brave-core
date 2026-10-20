// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/tcp_connector.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/net_errors.h"
#include "services/network/test/test_network_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

enum class Reply { kSuccess, kFailure, kNever };

class FakeNetworkContext : public network::TestNetworkContext {
 public:
  void CreateTCPConnectedSocket(
      const std::optional<net::IPEndPoint>& local_addr,
      const net::AddressList& remote_addr_list,
      network::mojom::TCPConnectedSocketOptionsPtr options,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojo::PendingReceiver<network::mojom::TCPConnectedSocket> socket,
      mojo::PendingRemote<network::mojom::SocketObserver> observer,
      CreateTCPConnectedSocketCallback callback) override {
    addresses = remote_addr_list;
    socket_receiver = std::move(socket);
    if (reply == Reply::kNever) {
      pending_callback = std::move(callback);
      return;
    }
    if (reply == Reply::kFailure) {
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE,
          base::BindOnce(std::move(callback), net::ERR_CONNECTION_REFUSED,
                         std::nullopt, std::nullopt,
                         mojo::ScopedDataPipeConsumerHandle(),
                         mojo::ScopedDataPipeProducerHandle()));
      return;
    }
    mojo::ScopedDataPipeConsumerHandle receive_consumer;
    mojo::ScopedDataPipeProducerHandle send_producer;
    ASSERT_EQ(mojo::CreateDataPipe(1024, receive_producer, receive_consumer),
              MOJO_RESULT_OK);
    ASSERT_EQ(mojo::CreateDataPipe(1024, send_producer, send_consumer),
              MOJO_RESULT_OK);
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), net::OK, std::nullopt,
                       remote_addr_list.front(), std::move(receive_consumer),
                       std::move(send_producer)));
  }

  Reply reply = Reply::kSuccess;
  net::AddressList addresses;
  mojo::PendingReceiver<network::mojom::TCPConnectedSocket> socket_receiver;
  CreateTCPConnectedSocketCallback pending_callback;
  mojo::ScopedDataPipeProducerHandle receive_producer;
  mojo::ScopedDataPipeConsumerHandle send_consumer;
};

}  // namespace

class BraveTransferTcpConnectorTest : public testing::Test {
 protected:
  network::mojom::NetworkContext* GetNetworkContext() {
    return has_network_context_ ? &network_context_ : nullptr;
  }

  std::unique_ptr<ByteStream> Connect(
      const std::vector<PeerEndpoint>& endpoints) {
    base::test::TestFuture<std::unique_ptr<ByteStream>> future;
    connector_.Connect(endpoints, future.GetCallback());
    return future.Take();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  FakeNetworkContext network_context_;
  bool has_network_context_ = true;
  TcpConnector connector_{
      base::BindRepeating(&BraveTransferTcpConnectorTest::GetNetworkContext,
                          base::Unretained(this))};
};

TEST(BraveTransferTcpConnectorAddressTest, KeepsIpLiteralsInOrder) {
  net::AddressList list = AddressListFromEndpoints({{"iPhone.local", 52000},
                                                    {"192.168.1.20", 52000},
                                                    {"fe80::1", 52000},
                                                    {"192.168.1.20", 52000},
                                                    {"192.168.1.21", 0}});
  ASSERT_EQ(list.size(), 2u);
  EXPECT_EQ(list[0].ToString(), "192.168.1.20:52000");
  EXPECT_EQ(list[1].ToString(), "[fe80::1]:52000");
}

TEST_F(BraveTransferTcpConnectorTest, Connects) {
  std::unique_ptr<ByteStream> stream =
      Connect({{"iPhone.local", 52000}, {"192.168.1.20", 52000}});
  ASSERT_TRUE(stream);
  ASSERT_EQ(network_context_.addresses.size(), 1u);
  EXPECT_EQ(network_context_.addresses[0].ToString(), "192.168.1.20:52000");
  EXPECT_TRUE(network_context_.socket_receiver.is_valid());
}

TEST_F(BraveTransferTcpConnectorTest, ConnectionFailure) {
  network_context_.reply = Reply::kFailure;
  EXPECT_FALSE(Connect({{"192.168.1.20", 52000}}));
}

TEST_F(BraveTransferTcpConnectorTest, NoUsableEndpoint) {
  EXPECT_FALSE(Connect({{"iPhone.local", 52000}}));
  EXPECT_FALSE(network_context_.socket_receiver.is_valid());
}

TEST_F(BraveTransferTcpConnectorTest, NoNetworkContext) {
  has_network_context_ = false;
  EXPECT_FALSE(Connect({{"192.168.1.20", 52000}}));
}

TEST_F(BraveTransferTcpConnectorTest, TimesOut) {
  network_context_.reply = Reply::kNever;
  base::test::TestFuture<std::unique_ptr<ByteStream>> future;
  connector_.Connect({{"192.168.1.20", 52000}}, future.GetCallback());
  task_environment_.FastForwardBy(TcpConnector::kConnectTimeout -
                                  base::Milliseconds(1));
  EXPECT_FALSE(future.IsReady());
  task_environment_.FastForwardBy(base::Milliseconds(1));
  ASSERT_TRUE(future.IsReady());
  EXPECT_FALSE(future.Take());
}

}  // namespace brave_transfer
