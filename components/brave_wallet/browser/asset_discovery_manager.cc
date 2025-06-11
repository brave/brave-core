/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include <algorithm>
#include <map>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/common_utils.h"
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
    BraveWalletService& wallet_service,
    JsonRpcService& json_rpc_service,
    KeyringService& keyring_service,
    SimpleHashClient& simple_hash_client,
    PrefService* prefs)
    : api_request_helper_(std::make_unique<APIRequestHelper>(
          GetAssetDiscoveryManagerNetworkTrafficAnnotationTag(),
          url_loader_factory)),
      wallet_service_(wallet_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      simple_hash_client_(simple_hash_client),
      prefs_(prefs),
      weak_ptr_factory_(this) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

AssetDiscoveryManager::~AssetDiscoveryManager() = default;

void AssetDiscoveryManager::DiscoverAssetsOnAllSupportedChains(
    std::vector<mojom::AccountIdPtr> accounts,
    bool bypass_rate_limit) {
  if (bypass_rate_limit) {
    AddTask(std::move(accounts));
    return;
  }

  // Check if there's already an in-flight asset discovery task
  if (!queue_.empty()) {
    return;
  }

  // Check if request should be throttled based on
  // kBraveWalletLastDiscoveredAssetsAt
  const base::Time assets_last_discovered_at =
      prefs_->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  if (!assets_last_discovered_at.is_null() &&
      ((base::Time::Now() - base::Minutes(kAssetDiscoveryMinutesPerRequest)) <
       assets_last_discovered_at)) {
    return;
  }
  prefs_->SetTime(kBraveWalletLastDiscoveredAssetsAt, base::Time::Now());

  AddTask(std::move(accounts));
}

std::vector<mojom::ChainIdPtr>
AssetDiscoveryManager::GetFungibleSupportedChains() {
  std::vector<mojom::ChainIdPtr> supported_chains;
  for (const auto& entry : kEthBalanceScannerContractAddresses) {
    supported_chains.push_back(
        mojom::ChainId::New(mojom::CoinType::ETH, std::string(entry.first)));
  }

  supported_chains.push_back(
      mojom::ChainId::New(mojom::CoinType::SOL, mojom::kSolanaMainnet));

  return supported_chains;
}

std::vector<mojom::ChainIdPtr>
AssetDiscoveryManager::GetNonFungibleSupportedChains() {
  // Use fungible chains as a base.
  std::vector<mojom::ChainIdPtr> non_fungible_supported_chains =
      CloneVector(GetFungibleSupportedChains());

  // Add in all the user networks that are supported by SimpleHash
  auto custom_non_fungible_eth_chains =
      wallet_service_->network_manager()->CustomChainsExist(
          {
              mojom::kArbitrumNovaChainId,
              mojom::kGnosisChainId,
              mojom::kGodwokenChainId,
              mojom::kPalmChainId,
              mojom::kPolygonZKEVMChainId,
              mojom::kZkSyncEraChainId,
          },
          mojom::CoinType::ETH);

  for (auto custom_chain : custom_non_fungible_eth_chains) {
    non_fungible_supported_chains.push_back(
        mojom::ChainId::New(mojom::CoinType::ETH, custom_chain));
  }

  std::ranges::sort(non_fungible_supported_chains);
  auto repeated = std::ranges::unique(non_fungible_supported_chains);
  non_fungible_supported_chains.erase(repeated.begin(), repeated.end());

  return non_fungible_supported_chains;
}

void AssetDiscoveryManager::AddTask(std::vector<mojom::AccountIdPtr> accounts) {
  auto fungible_supported_chains = GetFungibleSupportedChains();
  auto non_fungible_supported_chains = GetNonFungibleSupportedChains();

  auto task = std::make_unique<AssetDiscoveryTask>(
      *api_request_helper_, *simple_hash_client_, *wallet_service_,
      *json_rpc_service_, prefs_);
  auto callback = base::BindOnce(&AssetDiscoveryManager::FinishTask,
                                 weak_ptr_factory_.GetWeakPtr());
  auto* task_ptr = task.get();
  queue_.push(std::move(task));

  task_ptr->ScheduleTask(
      std::move(accounts), std::move(fungible_supported_chains),
      std::move(non_fungible_supported_chains), std::move(callback));
}

void AssetDiscoveryManager::FinishTask() {
  queue_.pop();
}

void AssetDiscoveryManager::AccountsAdded(
    std::vector<mojom::AccountInfoPtr> added_accounts) {
  std::vector<mojom::AccountIdPtr> accounts;
  for (const auto& account : added_accounts) {
    auto& account_id = account->account_id;
    if (account->account_id->coin == mojom::CoinType::ETH ||
        account->account_id->coin == mojom::CoinType::SOL) {
      accounts.push_back(account_id.Clone());
    }
  }

  if (accounts.empty()) {
    return;
  }

  DiscoverAssetsOnAllSupportedChains(std::move(accounts), true);
}

}  // namespace brave_wallet
