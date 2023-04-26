/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction_database.h"

#include <sstream>
#include <utility>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

namespace {

void FindUnspentOutputs(const std::string& address,
                        const std::set<bitcoin::Transaction>& transactions,
                        std::vector<bitcoin::Output>& result) {
  std::map<bitcoin::Outpoint, const bitcoin::Output*> outputs;

  // Search for outpoints to address and save them.
  for (const auto& tx : transactions) {
    const bitcoin::Transaction& tr = tx;
    for (const auto& o : tr.vout) {
      if (o.scriptpubkey_address != address) {
        continue;
      }

      outputs[o.outpoint] = &o;
    }
  }

  // Consider ouptpoints for inputs from address as spent and remove them.
  for (const auto& tx : transactions) {
    const bitcoin::Transaction& tr = tx;
    for (const auto& i : tr.vin) {
      if (i.scriptpubkey_address != address) {
        continue;
      }

      if (!outputs.erase(i.outpoint)) {
        NOTREACHED() << "No output to spend for outpoint "
                     << i.outpoint.txid_hex() << ":" << i.outpoint.index;
      }
    }
  }

  // Now there are only unspent outputs.
  for (auto& o : outputs) {
    result.push_back(*o.second);
  }
}

}  // namespace

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
    std::vector<bitcoin::Transaction> transactions) {
  for (auto& tr : transactions) {
    auto txid = tr.txid;

    // TODO(apaymyshev): should check that transaction with same txid in set
    // fully matches incoming one?

    transactions_[address].insert(tr);
  }

  return true;
}

std::vector<bitcoin::Output> BitcoinTransactionDatabase::GetUnspentOutputs(
    const std::string& address) {
  std::vector<bitcoin::Output> result;

  auto it = transactions_.find(address);
  if (it == transactions_.end()) {
    return result;
  }

  FindUnspentOutputs(it->first, it->second, result);

  return result;
}

std::vector<bitcoin::Output>
BitcoinTransactionDatabase::GetAllUnspentOutputs() {
  std::vector<bitcoin::Output> result;

  for (auto& address_item : transactions_) {
    FindUnspentOutputs(address_item.first, address_item.second, result);
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
