/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WALLET_CONNECT_WALLET_CONNECT_CLIENT_H_
#define BRAVE_COMPONENTS_WALLET_CONNECT_WALLET_CONNECT_CLIENT_H_

#include "brave/components/wallet_connect/wallet_connect.mojom.h"
#include "brave/components/wallet_connect/websocket_adapter.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace wallet_connect {

class WalletConnectClient {
 public:
  WalletConnectClient();
  ~WalletConnectClient();

  bool Init(const std::string& uri);

 private:
  enum class State { kNone, kConnected, kSessionEstablished };
  void OnTunnelReady(bool success);
  void OnTunnelData(absl::optional<base::span<const uint8_t>>);

  State state_ = State::kNone;
  std::string client_id_;
  mojom::WalletConnectURIDataPtr wallet_connect_uri_data_;
  std::unique_ptr<WebSocketAdapter> websocket_client_;
  mojo::Remote<network::mojom::NetworkContext> network_context_;
};

}  // namespace wallet_connect

#endif  // BRAVE_COMPONENTS_WALLET_CONNECT_WALLET_CONNECT_CLIENT_H_
