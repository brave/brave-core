/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NETWORK_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NETWORK_MANAGER_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

class PrefService;
class GURL;

namespace brave_wallet {

class NetworkManager {
 public:
  explicit NetworkManager(PrefService* prefs);
  ~NetworkManager();
  NetworkManager(NetworkManager&) = delete;
  NetworkManager& operator=(NetworkManager&) = delete;

  static GURL GetUnstoppableDomainsRpcUrl(const std::string& chain_id);
  static GURL GetEnsRpcUrl();
  static GURL GetSnsRpcUrl();
  static std::vector<mojom::NetworkInfoPtr> GetAllKnownChains(
      mojom::CoinType coin);
  static mojom::NetworkInfoPtr GetKnownChain(const std::string& chain_id,
                                             mojom::CoinType coin);
  std::vector<mojom::NetworkInfoPtr> GetAllCustomChains(mojom::CoinType coin);
  std::vector<mojom::NetworkInfoPtr> GetAllChains();
  mojom::NetworkInfoPtr GetCustomChain(const std::string& chain_id,
                                       mojom::CoinType coin);
  mojom::NetworkInfoPtr GetChain(const std::string& chain_id,
                                 mojom::CoinType coin);
  bool KnownChainExists(const std::string& chain_id, mojom::CoinType coin);
  bool CustomChainExists(const std::string& custom_chain_id,
                         mojom::CoinType coin);
  std::vector<std::string> CustomChainsExist(

      const std::vector<std::string>& custom_chain_ids,
      mojom::CoinType coin);

  GURL GetNetworkURL(const std::string& chain_id, mojom::CoinType coin);
  GURL GetNetworkURL(mojom::CoinType coin,
                     const std::optional<url::Origin>& origin);

  std::optional<bool> IsEip1559Chain(const std::string& chain_id);
  void SetEip1559ForCustomChain(const std::string& chain_id,
                                std::optional<bool> is_eip1559);

  void AddCustomNetwork(const mojom::NetworkInfo& chain);

  void RemoveCustomNetwork(const std::string& chain_id, mojom::CoinType coin);

  std::vector<std::string> GetHiddenNetworks(mojom::CoinType coin);
  void AddHiddenNetwork(mojom::CoinType coin, const std::string& chain_id);
  void RemoveHiddenNetwork(mojom::CoinType coin, const std::string& chain_id);

  // Get/Set the current chain ID for coin from kBraveWalletSelectedNetworks
  // pref when origin is not presented. If origin is presented,
  // kBraveWalletSelectedNetworksPerOrigin will be used. In addition, if origin
  // is opaque, we will also fallback to kBraveWalletSelectedNetworks but it
  // will be read only, other non http/https scheme will fallback to r/w
  // kBraveWalletSelectedNetworks.
  std::string GetCurrentChainId(mojom::CoinType coin,
                                const std::optional<url::Origin>& origin);
  bool SetCurrentChainId(mojom::CoinType coin,
                         const std::optional<url::Origin>& origin,
                         const std::string& chain_id);

  // DEPRECATED 01/2024. For migration only.
  static std::string GetNetworkId_DEPRECATED(mojom::CoinType coin,
                                             const std::string& chain_id);
  // DEPRECATED 01/2024. For migration only.
  static std::optional<std::string> GetChainIdByNetworkId_DEPRECATED(
      const mojom::CoinType& coin,
      const std::string& network_id);

 private:
  raw_ptr<PrefService, DanglingUntriaged> prefs_ = nullptr;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NETWORK_MANAGER_H_
