/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction_database.h"

#include <utility>

#include "base/notreached.h"

namespace brave_wallet {

BitcoinTransactionDatabase::BitcoinTransactionDatabase() = default;
BitcoinTransactionDatabase::~BitcoinTransactionDatabase() = default;

void BitcoinTransactionDatabase::SetChainHeight(uint32_t chain_height) {
  chain_height_ = chain_height;
}

absl::optional<uint32_t> BitcoinTransactionDatabase::GetChainHeight() const {
  return chain_height_;
}

bool BitcoinTransactionDatabase::AddTransactions(
    const std::string& address,
    std::vector<Transaction> transactions) {
  for (auto& tr : transactions) {
    auto txid = tr.txid;

    // TODO(apaymyshev): should check that transaction with same txid in set
    // fully matches incoming one?

    transactions_[address].insert(tr);
  }

  return true;
}

std::vector<Output> BitcoinTransactionDatabase::GetUnspentOutputs(
    const std::string& address) {
  std::map<Outpoint, const Output*> outputs;

  for (auto& item : transactions_[address]) {
    const Transaction& tr = item;
    for (auto& o : tr.vout) {
      if (o.scriptpubkey_address != address) {
        continue;
      }

      outputs[o.outpoint] = &o;
    }
  }

  for (auto& item : transactions_[address]) {
    const Transaction& tr = item;
    for (auto& i : tr.vin) {
      if (i.scriptpubkey_address != address) {
        continue;
      }

      if (!outputs.erase(i.outpoint)) {
        NOTREACHED();
      }
    }
  }

  std::vector<Output> result;
  for (auto& o : outputs) {
    result.push_back(*o.second);
  }

  return result;
}

std::vector<Output> BitcoinTransactionDatabase::GetAllUnspentOutputs() {
  std::map<Outpoint, const Output*> outputs;

  for (auto& address_item : transactions_) {
    auto& address = address_item.first;
    for (auto& item : address_item.second) {
      const Transaction& tr = item;
      for (auto& o : tr.vout) {
        if (o.scriptpubkey_address != address) {
          continue;
        }

        outputs[o.outpoint] = &o;
      }
    }

    for (auto& item : address_item.second) {
      const Transaction& tr = item;
      for (auto& i : tr.vin) {
        if (i.scriptpubkey_address != address) {
          continue;
        }

        if (!outputs.erase(i.outpoint)) {
          NOTREACHED();
        }
      }
    }
  }

  std::vector<Output> result;
  for (auto& o : outputs) {
    result.push_back(*o.second);
  }

  return result;
}

uint64_t BitcoinTransactionDatabase::GetBalance(const std::string& address) {
  uint64_t balance = 0;
  for (auto& uo : GetUnspentOutputs(address)) {
    balance += uo.value;
  }

  return balance;
}

}  // namespace brave_wallet
