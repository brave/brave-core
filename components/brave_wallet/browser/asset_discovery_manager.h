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

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_discovery_task.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

class PrefService;

namespace brave_wallet {

class BraveWalletService;
class JsonRpcService;
class KeyringService;

class AssetDiscoveryManager : public mojom::KeyringServiceObserver {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  AssetDiscoveryManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      BraveWalletService* wallet_service,
      JsonRpcService* json_rpc_service,
      KeyringService* keyring_service,
      PrefService* prefs);

  AssetDiscoveryManager(const AssetDiscoveryManager&) = delete;
  AssetDiscoveryManager& operator=(AssetDiscoveryManager&) = delete;
  ~AssetDiscoveryManager() override;

  // KeyringServiceObserver
  void KeyringCreated(const std::string& keyring_id) override {}
  void KeyringRestored(const std::string& keyring_id) override {}
  void KeyringReset() override {}
  void Locked() override {}
  void Unlocked() override {}
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AccountsAdded(mojom::CoinType coin,
                     const std::vector<std::string>& addresses) override;
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged(mojom::CoinType coin) override {}

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
                           GetAssetDiscoverySupportedChains);

  const std::map<mojom::CoinType, std::vector<std::string>>&
  GetAssetDiscoverySupportedChains();

  void AddTask(const std::map<mojom::CoinType, std::vector<std::string>>&
                   account_addresses);
  void FinishTask();

  std::queue<std::unique_ptr<AssetDiscoveryTask>> queue_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  raw_ptr<BraveWalletService> wallet_service_;
  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<PrefService> prefs_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};
  base::WeakPtrFactory<AssetDiscoveryManager> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_DISCOVERY_MANAGER_H_
