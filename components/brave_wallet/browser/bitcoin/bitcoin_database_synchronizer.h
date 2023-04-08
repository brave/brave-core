/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_DATABASE_SYNCHRONIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_DATABASE_SYNCHRONIZER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction_database.h"

namespace brave_wallet {

// TODO(apaymyshev): test this class
class BitcoinDatabaseSynchronizer {
 public:
  BitcoinDatabaseSynchronizer(const std::string& network_id,
                              BitcoinRpc* bitcoin_rpc,
                              BitcoinTransactionDatabase* database);
  ~BitcoinDatabaseSynchronizer();

  void Start(const std::vector<std::string>& addresses);
  void AddWatchAddresses(const std::vector<std::string>& addresses);

 private:
  struct WatchedAddressData {
    std::string newest_txid;
    std::string oldest_txid;
  };

  void FetchChainHeight();
  void OnFetchChainHeight(base::expected<uint32_t, std::string> height);

  void SyncAllAddresses();
  void SyncAddress(const std::string& address, const uint32_t max_block_height);
  void FetchAddressHistory(const std::string& address,
                           const uint32_t max_block_height,
                           const std::string& last_seen_txid_filter);
  void OnFetchAddressHistory(
      const std::string& address,
      const uint32_t max_block_height,
      const std::string& last_seen_txid_filter,
      base::expected<std::vector<Transaction>, std::string> transactions);

  std::map<std::string, WatchedAddressData> addresses_;

  base::RepeatingTimer timer_;
  std::string network_id_;
  raw_ptr<BitcoinRpc> bitcoin_rpc_ = nullptr;
  raw_ptr<BitcoinTransactionDatabase> database_ = nullptr;
  base::WeakPtrFactory<BitcoinDatabaseSynchronizer> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_DATABASE_SYNCHRONIZER_H_
