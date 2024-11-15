/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_DISCOVERY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_DISCOVERY_MANAGER_H_

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_discovery_task.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/simple_hash_client.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

class PrefService;

namespace brave_wallet {

class BraveWalletService;
class JsonRpcService;
class SimpleHashClient;
class KeyringService;

class AssetDiscoveryManager : public KeyringServiceObserverBase {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  AssetDiscoveryManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      BraveWalletService& wallet_service,
      JsonRpcService& json_rpc_service,
      KeyringService& keyring_service,
      SimpleHashClient& simple_hash_client,
      PrefService* prefs);

  AssetDiscoveryManager(const AssetDiscoveryManager&) = delete;
  AssetDiscoveryManager& operator=(AssetDiscoveryManager&) = delete;
  ~AssetDiscoveryManager() override;

  // KeyringServiceObserverBase:
  void AccountsAdded(
      std::vector<mojom::AccountInfoPtr> added_accounts) override;

  // Called by frontend via BraveWalletService and when new accounts are added
  // via the KeyringServiceObserver implementation
  void DiscoverAssetsOnAllSupportedChains(
      const std::map<mojom::CoinType, std::vector<std::string>>&
          account_addresses,
      bool bypass_rate_limit);

  void SetQueueForTesting(
      std::queue<std::unique_ptr<AssetDiscoveryTask>> queue) {
    queue_ = std::move(queue);
  }

  size_t GetQueueSizeForTesting() { return queue_.size(); }

 private:
  friend class AssetDiscoveryManagerUnitTest;
  FRIEND_TEST_ALL_PREFIXES(AssetDiscoveryManagerUnitTest,
                           GetFungibleSupportedChains);
  FRIEND_TEST_ALL_PREFIXES(AssetDiscoveryManagerUnitTest,
                           GetNonFungibleSupportedChains);

  const std::map<mojom::CoinType, std::vector<std::string>>&
  GetFungibleSupportedChains();
  const std::map<mojom::CoinType, std::vector<std::string>>
  GetNonFungibleSupportedChains();

  void AddTask(const std::map<mojom::CoinType, std::vector<std::string>>&
                   account_addresses);
  void FinishTask();

  std::unique_ptr<APIRequestHelper> api_request_helper_;
  std::queue<std::unique_ptr<AssetDiscoveryTask>> queue_;
  raw_ref<BraveWalletService> wallet_service_;
  raw_ref<JsonRpcService> json_rpc_service_;
  raw_ref<KeyringService> keyring_service_;
  raw_ref<SimpleHashClient> simple_hash_client_;
  raw_ptr<PrefService> prefs_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};
  base::WeakPtrFactory<AssetDiscoveryManager> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_DISCOVERY_MANAGER_H_
