/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include <map>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

net::NetworkTrafficAnnotationTag
GetAssetDiscoveryManagerNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_wallet_service", R"(
      semantics {
        sender: "Asset Discovery Manager"
        description:
          "This service is used to discover crypto assets on behalf "
          "of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "NFT assets."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

namespace brave_wallet {

AssetDiscoveryManager::AssetDiscoveryManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    BraveWalletService* wallet_service,
    JsonRpcService* json_rpc_service,
    KeyringService* keyring_service,
    PrefService* prefs)
    : api_request_helper_(std::make_unique<APIRequestHelper>(
          GetAssetDiscoveryManagerNetworkTrafficAnnotationTag(),
          url_loader_factory)),
      wallet_service_(wallet_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs),
      weak_ptr_factory_(this) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

AssetDiscoveryManager::~AssetDiscoveryManager() = default;

void AssetDiscoveryManager::DiscoverAssetsOnAllSupportedChains(
    const std::map<mojom::CoinType, std::vector<std::string>>&
        account_addresses,
    bool triggered_by_accounts_added) {
  if (triggered_by_accounts_added) {
    // Always add asset discovery when an account is added
    AddTask(account_addresses);
    return;
  }

  // Check if there's already an in-flight asset discovery task
  if (!queue_.empty()) {
    return;
  }

  // Check if request should be rate limited throttled based on last
  // kBraveWalletLastDiscoveredAssetsAt
  const base::Time assets_last_discovered_at =
      prefs_->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  if (!assets_last_discovered_at.is_null() &&
      ((base::Time::Now() - base::Minutes(kAssetDiscoveryMinutesPerRequest)) <
       assets_last_discovered_at)) {
    return;
  }
  prefs_->SetTime(kBraveWalletLastDiscoveredAssetsAt, base::Time::Now());

  AddTask(account_addresses);
}

const std::map<mojom::CoinType, std::vector<std::string>>&
AssetDiscoveryManager::GetAssetDiscoverySupportedChains() {
  static const base::NoDestructor<
      std::map<mojom::CoinType, std::vector<std::string>>>
      asset_discovery_supported_chains([] {
        std::map<mojom::CoinType, std::vector<std::string>> supported_chains;
        std::vector<std::string> supported_eth_chains;
        for (const auto& entry : GetEthBalanceScannerContractAddresses()) {
          supported_eth_chains.push_back(entry.first);
        }
        supported_chains[mojom::CoinType::ETH] =
            std::move(supported_eth_chains);

        std::vector<std::string> supported_sol_chains;
        supported_sol_chains.push_back(mojom::kSolanaMainnet);
        supported_chains[mojom::CoinType::SOL] =
            std::move(supported_sol_chains);

        return supported_chains;
      }());
  return *asset_discovery_supported_chains;
}

void AssetDiscoveryManager::AddTask(
    const std::map<mojom::CoinType, std::vector<std::string>>&
        account_addresses) {
  auto task = std::make_unique<AssetDiscoveryTask>(
      api_request_helper_.get(), wallet_service_, json_rpc_service_, prefs_);
  auto callback = base::BindOnce(&AssetDiscoveryManager::FinishTask,
                                 weak_ptr_factory_.GetWeakPtr());
  auto* task_ptr = task.get();
  queue_.push(std::move(task));
  task_ptr->ScheduleTask(GetAssetDiscoverySupportedChains(), account_addresses,
                         std::move(callback));
}

void AssetDiscoveryManager::FinishTask() {
  queue_.pop();
}

void AssetDiscoveryManager::AccountsAdded(
    mojom::CoinType coin,
    const std::vector<std::string>& addresses) {
  if (!(coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL) ||
      addresses.empty()) {
    return;
  }

  std::map<mojom::CoinType, std::vector<std::string>> account_addresses_map;
  account_addresses_map[coin] = std::move(addresses);
  DiscoverAssetsOnAllSupportedChains(account_addresses_map, true);
}

}  // namespace brave_wallet
