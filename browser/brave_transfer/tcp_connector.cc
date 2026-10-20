// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/tcp_connector.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/brave_transfer/mojo_byte_stream.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace brave_transfer {

namespace {

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_transfer", R"(
      semantics {
        sender: "Brave Transfer"
        description:
          "Connects to the user's own devices in their Brave Sync chain on "
          "the local network to send and receive files, photos, text and "
          "links. The connection is end-to-end encrypted with keys derived "
          "from the Sync chain."
        trigger:
          "The brave://transfer page is open and a device in the user's "
          "Sync chain is found on the local network."
        data:
          "Items the user chose to send, device names and protocol messages."
        destination: OTHER
        destination_other: "The user's own devices on the local network."
      }
      policy {
        cookies_allowed: NO
        setting:
          "Brave Transfer can be turned off on the brave://transfer page, "
          "and is only active while Brave Sync is on."
        policy_exception_justification: "Controlled by BraveTransferEnabled."
      })");

}  // namespace

net::AddressList AddressListFromEndpoints(
    const std::vector<PeerEndpoint>& endpoints) {
  net::AddressList list;
  for (const auto& endpoint : endpoints) {
    net::IPAddress address;
    if (endpoint.port == 0 || !address.AssignFromIPLiteral(endpoint.host)) {
      continue;
    }
    net::IPEndPoint ip_endpoint(address, endpoint.port);
    if (!list.empty() && std::ranges::find(list.endpoints(), ip_endpoint) !=
                             list.endpoints().end()) {
      continue;
    }
    list.push_back(std::move(ip_endpoint));
  }
  return list;
}

TcpConnector::Attempt::Attempt() = default;
TcpConnector::Attempt::Attempt(Attempt&&) = default;
TcpConnector::Attempt& TcpConnector::Attempt::operator=(Attempt&&) = default;
TcpConnector::Attempt::~Attempt() = default;

TcpConnector::TcpConnector(NetworkContextGetter network_context_getter)
    : network_context_getter_(std::move(network_context_getter)) {}

TcpConnector::~TcpConnector() = default;

void TcpConnector::Connect(const std::vector<PeerEndpoint>& endpoints,
                           ConnectCallback callback) {
  net::AddressList addresses = AddressListFromEndpoints(endpoints);
  network::mojom::NetworkContext* network_context =
      network_context_getter_.Run();
  if (addresses.empty() || !network_context) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), nullptr));
    return;
  }

  const int attempt_id = next_attempt_id_++;
  Attempt& attempt = attempts_[attempt_id];
  attempt.callback = std::move(callback);
  mojo::PendingRemote<network::mojom::SocketObserver> observer;
  attempt.observer = observer.InitWithNewPipeAndPassReceiver();
  attempt.timeout = std::make_unique<base::OneShotTimer>();
  attempt.timeout->Start(FROM_HERE, kConnectTimeout,
                         base::BindOnce(&TcpConnector::OnTimeout,
                                        base::Unretained(this), attempt_id));

  network_context->CreateTCPConnectedSocket(
      /*local_addr=*/std::nullopt, addresses,
      /*tcp_connected_socket_options=*/nullptr,
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation),
      attempt.socket.BindNewPipeAndPassReceiver(), std::move(observer),
      base::BindOnce(&TcpConnector::OnConnected, weak_factory_.GetWeakPtr(),
                     attempt_id));
}

void TcpConnector::OnConnected(
    int attempt_id,
    int32_t result,
    const std::optional<net::IPEndPoint>& local_addr,
    const std::optional<net::IPEndPoint>& peer_addr,
    mojo::ScopedDataPipeConsumerHandle receive_stream,
    mojo::ScopedDataPipeProducerHandle send_stream) {
  auto it = attempts_.find(attempt_id);
  if (it == attempts_.end()) {
    return;
  }
  Attempt attempt = std::move(it->second);
  attempts_.erase(it);

  if (result != net::OK || !receive_stream || !send_stream) {
    std::move(attempt.callback).Run(nullptr);
    return;
  }
  std::move(attempt.callback)
      .Run(std::make_unique<MojoByteStream>(
          attempt.socket.Unbind(), std::move(attempt.observer),
          std::move(receive_stream), std::move(send_stream)));
}

void TcpConnector::OnTimeout(int attempt_id) {
  auto it = attempts_.find(attempt_id);
  if (it == attempts_.end()) {
    return;
  }
  ConnectCallback callback = std::move(it->second.callback);
  attempts_.erase(it);
  std::move(callback).Run(nullptr);
}

}  // namespace brave_transfer
