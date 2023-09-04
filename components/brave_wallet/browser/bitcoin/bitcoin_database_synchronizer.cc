/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_database_synchronizer.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

BitcoinDatabaseSynchronizer::BitcoinDatabaseSynchronizer(
    const std::string& network_id,
    BitcoinRpc* bitcoin_rpc,
    BitcoinTransactionDatabase* database)
    : network_id_(network_id), bitcoin_rpc_(bitcoin_rpc), database_(database) {
  DCHECK(IsBitcoinNetwork(network_id_));
}

BitcoinDatabaseSynchronizer::~BitcoinDatabaseSynchronizer() = default;

void BitcoinDatabaseSynchronizer::Start(
    const std::vector<std::string>& addresses) {
  AddWatchAddresses(addresses);
  FetchChainHeight();

  // TODO(apaymyshev): this is similar to BitcoinBlockTracker, but needs to be
  // active without pending transactions. Think how to keep only one.
  timer_.Start(
      FROM_HERE, base::Seconds(10),
      base::BindRepeating(&BitcoinDatabaseSynchronizer::FetchChainHeight,
                          weak_ptr_factory_.GetWeakPtr()));
}

void BitcoinDatabaseSynchronizer::AddWatchAddresses(
    const std::vector<std::string>& addresses) {
  const auto& current_height = database_->GetChainHeight();

  for (const auto& a : addresses) {
    if (addresses_.contains(a)) {
      continue;
    }

    addresses_.insert({a, WatchedAddressData()});
    if (current_height) {
      SyncAddress(a, current_height.value());
    }
  }
}

void BitcoinDatabaseSynchronizer::FetchChainHeight() {
  bitcoin_rpc_->GetChainHeight(
      network_id_,
      base::BindOnce(&BitcoinDatabaseSynchronizer::OnFetchChainHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BitcoinDatabaseSynchronizer::OnFetchChainHeight(
    base::expected<uint32_t, std::string> height) {
  if (height.has_value()) {
    const auto& current_height = database_->GetChainHeight();
    database_->SetChainHeight(height.value());

    // New block in chain - update transactions history for all watched
    // addresses.
    if (current_height != height.value()) {
      SyncAllAddresses();
    }
  }
}

void BitcoinDatabaseSynchronizer::SyncAllAddresses() {
  if (auto max_block_height = database_->GetChainHeight()) {
    for (const auto& a : addresses_) {
      SyncAddress(a.first, max_block_height.value());
    }
  }
}

void BitcoinDatabaseSynchronizer::SyncAddress(const std::string& address,
                                              const uint32_t max_block_height) {
  FetchAddressHistory(address, max_block_height, "");
}

void BitcoinDatabaseSynchronizer::FetchAddressHistory(
    const std::string& address,
    const uint32_t max_block_height,
    const std::string& last_seen_txid_filter) {
  bitcoin_rpc_->GetAddressHistory(
      network_id_, address, max_block_height, last_seen_txid_filter,
      base::BindOnce(&BitcoinDatabaseSynchronizer::OnFetchAddressHistory,
                     weak_ptr_factory_.GetWeakPtr(), address, max_block_height,
                     last_seen_txid_filter));
}

void BitcoinDatabaseSynchronizer::OnFetchAddressHistory(
    const std::string& address,
    const uint32_t max_block_height,
    const std::string& last_seen_txid_filter,
    base::expected<std::vector<bitcoin::Transaction>, std::string>
        transactions) {
  if (!transactions.has_value()) {
    return;
  }

  // TODO(apaymyshev): stop fetching transaction history when no new entries are
  // expected.

  if (!transactions.value().empty()) {
    if (last_seen_txid_filter.empty()) {
      addresses_[address].newest_txid = transactions.value().front().txid;
    }
  }

  if (transactions.value().empty()) {
    if (!last_seen_txid_filter.empty()) {
      addresses_[address].oldest_txid = last_seen_txid_filter;
    }
    return;
  }

  auto new_last_seen_txid_filter = transactions.value().back().txid;
  database_->AddTransactions(address, std::move(transactions.value()));

  FetchAddressHistory(address, max_block_height, new_last_seen_txid_filter);
}

}  // namespace brave_wallet
