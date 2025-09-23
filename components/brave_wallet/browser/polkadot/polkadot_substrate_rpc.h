/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_RPC_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class NetworkManager;

// The main driver for the Polkadot-based RPC calls against the relay chain and
// the Substrate-based parachains.
class PolkadotSubstrateRpc {
 public:
  explicit PolkadotSubstrateRpc(
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~PolkadotSubstrateRpc();

  using GetChainNameCallback =
      base::OnceCallback<void(const std::optional<std::string>&,
                              const std::optional<std::string>&)>;

  // Get the name of the chain pointed to by the current network configuration.
  // "Westend" or "Paseo" for the testnets, "Polkadot" for the mainnet.
  void GetChainName(const std::string& chain_id, GetChainNameCallback callback);

 private:
  GURL GetNetworkURL(const std::string& chain_id);

  using APIRequestResult = api_request_helper::APIRequestResult;

  void OnGetChainName(GetChainNameCallback callback, APIRequestResult res);

  const raw_ref<NetworkManager> network_manager_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<PolkadotSubstrateRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_RPC_H_
