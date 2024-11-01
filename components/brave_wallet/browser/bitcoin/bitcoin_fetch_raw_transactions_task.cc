/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_fetch_raw_transactions_task.h"

#include <stdint.h>

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

FetchRawTransactionsTask::FetchRawTransactionsTask(
    BitcoinWalletService& bitcoin_wallet_service,
    const std::string& network_id,
    const std::vector<SHA256HashArray>& txids)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      network_id_(network_id),
      txids_(txids) {
  CHECK(IsBitcoinNetwork(network_id));
}
FetchRawTransactionsTask::~FetchRawTransactionsTask() = default;

void FetchRawTransactionsTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&FetchRawTransactionsTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void FetchRawTransactionsTask::MaybeQueueRequests() {
  if (!raw_transactions_.empty()) {
    return;
  }

  for (auto& txid : txids_) {
    if (raw_transactions_.contains(txid)) {
      continue;
    }

    raw_transactions_[txid] = {};

    bitcoin_wallet_service_->bitcoin_rpc().GetTransactionRaw(
        network_id_, base::HexEncode(txid),
        base::BindOnce(&FetchRawTransactionsTask::OnGetTransactionRaw,
                       weak_ptr_factory_.GetWeakPtr(), txid));
    active_requests_++;
  }
}

void FetchRawTransactionsTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  MaybeQueueRequests();

  if (active_requests_) {
    return;
  }

  std::vector<std::vector<uint8_t>> result;
  for (auto& txid : txids_) {
    result.push_back(raw_transactions_[txid]);
    DCHECK(!result.back().empty());
  }
  std::move(callback_).Run(base::ok(std::move(result)));
}

void FetchRawTransactionsTask::OnGetTransactionRaw(
    SHA256HashArray txid,
    base::expected<std::vector<uint8_t>, std::string> raw_tx) {
  DCHECK_GT(active_requests_, 0u);
  active_requests_--;

  if (!raw_tx.has_value()) {
    error_ = raw_tx.error();
    WorkOnTask();
    return;
  }

  auto& raw_tx_value = raw_transactions_[txid];
  DCHECK(raw_tx_value.empty());
  raw_tx_value = std::move(raw_tx.value());
  DCHECK(!raw_tx_value.empty());

  WorkOnTask();
}

}  // namespace brave_wallet
