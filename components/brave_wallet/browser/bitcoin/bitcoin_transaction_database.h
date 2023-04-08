/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_DATABASE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"

namespace brave_wallet {

// TODO(apaymyshev): tests for this class
class BitcoinTransactionDatabase {
 public:
  BitcoinTransactionDatabase();
  ~BitcoinTransactionDatabase();
  BitcoinTransactionDatabase(BitcoinTransactionDatabase& other) = delete;
  BitcoinTransactionDatabase& operator=(BitcoinTransactionDatabase& other) =
      delete;

  void SetChainHeight(uint32_t chain_height);
  absl::optional<uint32_t> GetChainHeight() const;
  bool AddTransactions(const std::string& address,
                       std::vector<Transaction> transactions);
  std::vector<Output> GetUnspentOutputs(const std::string& address);
  std::vector<Output> GetAllUnspentOutputs();
  uint64_t GetBalance(const std::string& address);

 private:
  absl::optional<uint32_t> chain_height_;
  // TODO(apaymyshev): avoid transaction duplicates for different addresses?
  std::map<std::string, std::set<Transaction>> transactions_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_DATABASE_H_
