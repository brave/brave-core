/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_DISCOVERY_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_DISCOVERY_TASK_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/simple_hash_client.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/solana_address.h"

class PrefService;

namespace brave_wallet {

class BraveWalletService;
class JsonRpcService;

class AssetDiscoveryTask {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;

  AssetDiscoveryTask(APIRequestHelper& api_request_helper,
                     SimpleHashClient& simple_hash_client,
                     BraveWalletService& wallet_service,
                     JsonRpcService& json_rpc_service,
                     PrefService* prefs);

  AssetDiscoveryTask(const AssetDiscoveryTask&) = delete;
  AssetDiscoveryTask& operator=(AssetDiscoveryTask&) = delete;
  ~AssetDiscoveryTask();

  void ScheduleTask(const std::map<mojom::CoinType, std::vector<std::string>>&
                        fungible_chain_ids,
                    const std::map<mojom::CoinType, std::vector<std::string>>&
                        non_fungible_chain_ids,
                    const std::map<mojom::CoinType, std::vector<std::string>>&
                        account_addresses,
                    base::OnceClosure callback);

 private:
  friend class AssetDiscoveryTaskUnitTest;
  FRIEND_TEST_ALL_PREFIXES(AssetDiscoveryTaskUnitTest, DecodeMintAddress);
  FRIEND_TEST_ALL_PREFIXES(AssetDiscoveryTaskUnitTest,
                           GetSimpleHashNftsByWalletUrl);
  FRIEND_TEST_ALL_PREFIXES(AssetDiscoveryTaskUnitTest, ParseNFTsFromSimpleHash);

  void DiscoverAssets(const std::map<mojom::CoinType, std::vector<std::string>>&
                          fungible_chain_ids,
                      const std::map<mojom::CoinType, std::vector<std::string>>&
                          non_fungible_chain_ids,
                      const std::map<mojom::CoinType, std::vector<std::string>>&
                          account_addresses,
                      base::OnceClosure callback);

  void MergeDiscoveredAssets(
      base::OnceClosure callback,
      const std::vector<std::vector<mojom::BlockchainTokenPtr>>&
          discovered_assets);

  using DiscoverAssetsCompletedCallback =
      base::OnceCallback<void(std::vector<mojom::BlockchainTokenPtr> tokens)>;

  void DiscoverAnkrTokens(const std::vector<std::string>& chain_ids,
                          const std::vector<std::string>& account_addresses,
                          DiscoverAssetsCompletedCallback callback);
  void OnAnkrGetAccountBalances(
      base::OnceCallback<void(std::vector<mojom::AnkrAssetBalancePtr>)>
          barrier_callback,
      std::vector<mojom::AnkrAssetBalancePtr> balances,
      mojom::ProviderError error,
      const std::string& error_message);
  void MergeDiscoveredAnkrTokens(
      DiscoverAssetsCompletedCallback callback,
      const std::vector<std::vector<mojom::AnkrAssetBalancePtr>>&
          discovered_assets_results);

  void DiscoverERC20sFromRegistry(
      const std::vector<std::string>& chain_ids,
      const std::vector<std::string>& account_addresses,
      DiscoverAssetsCompletedCallback callback);
  void OnGetERC20TokenBalances(
      base::OnceCallback<void(std::map<std::string, std::vector<std::string>>)>
          barrier_callback,
      const std::string& chain_id,
      const std::vector<std::string>& contract_addresses,
      std::vector<mojom::ERC20BalanceResultPtr> balance_results,
      mojom::ProviderError error,
      const std::string& error_message);
  void MergeDiscoveredERC20s(
      base::flat_map<std::string,
                     base::flat_map<std::string, mojom::BlockchainTokenPtr>>
          chain_id_to_contract_address_to_token,
      DiscoverAssetsCompletedCallback callback,
      const std::vector<std::map<std::string, std::vector<std::string>>>&
          discovered_assets);

  void DiscoverSPLTokensFromRegistry(
      const std::vector<std::string>& account_addresses,
      DiscoverAssetsCompletedCallback callback);
  void OnGetSolanaTokenAccountsByOwner(
      base::OnceCallback<void(std::vector<SolanaAddress>)> barrier_callback,
      std::vector<SolanaAccountInfo> token_accounts,
      mojom::SolanaProviderError error,
      const std::string& error_message);
  void MergeDiscoveredSPLTokens(DiscoverAssetsCompletedCallback callback,
                                const std::vector<std::vector<SolanaAddress>>&
                                    all_discovered_contract_addresses);
  void OnGetSolanaTokenRegistry(
      DiscoverAssetsCompletedCallback callback,
      const base::flat_set<std::string>& discovered_contract_addresses,
      std::vector<mojom::BlockchainTokenPtr> sol_token_registry);

  void DiscoverNFTs(
      const std::map<mojom::CoinType, std::vector<std::string>>& chain_ids,
      const std::map<mojom::CoinType, std::vector<std::string>>&
          account_addresses,
      DiscoverAssetsCompletedCallback callback);
  void MergeDiscoveredNFTs(
      DiscoverAssetsCompletedCallback callback,
      const std::vector<std::vector<mojom::BlockchainTokenPtr>>& nfts);

  std::optional<std::pair<GURL, std::vector<mojom::BlockchainTokenPtr>>>
  ParseNFTsFromSimpleHash(const base::Value& json_value, mojom::CoinType coin);

  static std::optional<SolanaAddress> DecodeMintAddress(
      const std::vector<uint8_t>& data);

  raw_ref<APIRequestHelper> api_request_helper_;
  raw_ref<SimpleHashClient> simple_hash_client_;
  raw_ref<BraveWalletService> wallet_service_;
  raw_ref<JsonRpcService> json_rpc_service_;
  raw_ptr<PrefService> prefs_;
  base::WeakPtrFactory<AssetDiscoveryTask> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_DISCOVERY_TASK_H_
