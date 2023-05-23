/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ALLOWANCE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ALLOWANCE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "base/barrier_callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

class PrefService;

namespace brave_wallet {

class JsonRpcService;
class KeyringService;
class EthAllowanceManager {
 public:
  EthAllowanceManager(BraveWalletService* wallet_service,
                      JsonRpcService* json_rpc_service,
                      KeyringService* keyring_service,
                      PrefService* prefs);
  EthAllowanceManager(const EthAllowanceManager&) = delete;
  EthAllowanceManager& operator=(const EthAllowanceManager&) = delete;
  EthAllowanceManager(const EthAllowanceManager&&) = delete;
  EthAllowanceManager& operator=(const EthAllowanceManager&&) = delete;

  ~EthAllowanceManager();

  void DiscoverEthAllowancesOnAllSupportedChains(
      BraveWalletService::DiscoverEthAllowancesCallback callback);
  void Reset();

 private:
  friend class EthAllowanceManagerUnitTest;
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest, LoadCachedAllowances);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           CouldNotLoadCachedAllowancesPrefsEmpty);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           CouldNotLoadCachedAllowancesByAddress);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           CouldNotLoadCachedAllowancesIncorrectCacheData);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           BreakAllowanceDiscoveringIfTokenListEmpty);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest, AllowancesLoading);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest, AllowancesRevoked);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           AllowancesIgnorePendingBlocks);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           AllowancesIgnoreWrongTopicsData);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           AllowancesIgnoreWrongAmountData);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest, NoAllowancesLoaded);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           NoAllowancesLoadedForSkippedNetwork);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest, AllowancesLoadingReset);

  void OnGetAllowances(const int& task_id,
                       const std::string& chain_id,
                       const std::string& hex_account_address,
                       [[maybe_unused]] const std::vector<Log>& logs,
                       base::Value rawlogs,
                       mojom::ProviderError error,
                       const std::string& error_message);

  void LoadCachedAllowances(
      const std::string& chain_id,
      const std::string& hex_account_address,
      std::map<std::string, std::tuple<uint256_t, mojom::AllowanceInfoPtr>>&
          allowances_map);
  using EthAllowanceCollection =
      base::flat_map<std::string,
                     std::tuple<brave_wallet::uint256_t,
                                std::vector<mojom::AllowanceInfoPtr>>>;
  struct EthAllowanceTask {
    explicit EthAllowanceTask(const int& taskid);
    EthAllowanceTask(const EthAllowanceTask&) = delete;
    EthAllowanceTask& operator=(const EthAllowanceTask&) = delete;
    EthAllowanceTask(const EthAllowanceTask&&) = delete;
    EthAllowanceTask& operator=(const EthAllowanceTask&&) = delete;
    ~EthAllowanceTask();

    void SetResults(const std::string& chain_id,
                    const uint256_t& max_block_number,
                    std::vector<mojom::AllowanceInfoPtr>&& alwns);
    void MarkComplete();
    int task_id;
    EthAllowanceCollection allowances;
    bool is_completed;
  };
  bool IsAllTasksCompleted();
  void MergeAllResultsAnCallBack();
  void OnDiscoverEthAllowancesCompleted(
      std::vector<mojom::AllowanceInfoPtr> result);

  std::map<int, std::unique_ptr<EthAllowanceTask>> tasks;
  std::vector<BraveWalletService::DiscoverEthAllowancesCallback>
      discover_eth_allowance_callback_;
  raw_ptr<BraveWalletService> wallet_service_;
  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<PrefService> prefs_;

  base::WeakPtrFactory<EthAllowanceManager> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ALLOWANCE_MANAGER_H_
