// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_TRANSFER_TCP_CONNECTOR_H_
#define BRAVE_BROWSER_BRAVE_TRANSFER_TCP_CONNECTOR_H_

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_transfer/browser/connector.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/address_list.h"
#include "net/base/ip_endpoint.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"

namespace brave_transfer {

// Endpoints that are IP literals, in order. Host names are skipped: discovery
// reports resolved addresses, and the network service does not resolve
// names for raw TCP sockets.
net::AddressList AddressListFromEndpoints(
    const std::vector<PeerEndpoint>& endpoints);

// Dials discovered peers through the profile's network context.
class TcpConnector : public Connector {
 public:
  using NetworkContextGetter =
      base::RepeatingCallback<network::mojom::NetworkContext*()>;

  static constexpr base::TimeDelta kConnectTimeout = base::Seconds(10);

  explicit TcpConnector(NetworkContextGetter network_context_getter);
  TcpConnector(const TcpConnector&) = delete;
  TcpConnector& operator=(const TcpConnector&) = delete;
  ~TcpConnector() override;

  // Connector:
  void Connect(const std::vector<PeerEndpoint>& endpoints,
               ConnectCallback callback) override;

 private:
  struct Attempt {
    Attempt();
    Attempt(Attempt&&);
    Attempt& operator=(Attempt&&);
    ~Attempt();

    mojo::Remote<network::mojom::TCPConnectedSocket> socket;
    mojo::PendingReceiver<network::mojom::SocketObserver> observer;
    ConnectCallback callback;
    std::unique_ptr<base::OneShotTimer> timeout;
  };

  void OnConnected(int attempt_id,
                   int32_t result,
                   const std::optional<net::IPEndPoint>& local_addr,
                   const std::optional<net::IPEndPoint>& peer_addr,
                   mojo::ScopedDataPipeConsumerHandle receive_stream,
                   mojo::ScopedDataPipeProducerHandle send_stream);
  void OnTimeout(int attempt_id);

  NetworkContextGetter network_context_getter_;
  int next_attempt_id_ = 1;
  std::map<int, Attempt> attempts_;
  base::WeakPtrFactory<TcpConnector> weak_factory_{this};
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_BRAVE_TRANSFER_TCP_CONNECTOR_H_
