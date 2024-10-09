/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_FETCH_RAW_TRANSACTIONS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_FETCH_RAW_TRANSACTIONS_TASK_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

class BitcoinWalletService;

class FetchRawTransactionsTask {
 public:
  using FetchRawTransactionsTaskCallback = base::OnceCallback<void(
      base::expected<std::vector<std::vector<uint8_t>>, std::string>)>;

  FetchRawTransactionsTask(BitcoinWalletService* bitcoin_wallet_service,
                           const std::string& network_id,
                           const std::vector<SHA256HashArray>& txids);
  virtual ~FetchRawTransactionsTask();

  void ScheduleWorkOnTask();
  void set_callback(FetchRawTransactionsTaskCallback callback) {
    callback_ = std::move(callback);
  }

 protected:
  void MaybeQueueRequests();

  void WorkOnTask();
  void OnGetTransactionRaw(
      SHA256HashArray txid,
      base::expected<std::vector<uint8_t>, std::string> raw_tx);

  raw_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  std::string network_id_;
  std::vector<SHA256HashArray> txids_;
  bool requests_queued_ = false;
  uint32_t active_requests_ = 0;
  std::map<SHA256HashArray, std::vector<uint8_t>> raw_transactions_;

  std::optional<std::string> error_;
  FetchRawTransactionsTaskCallback callback_;
  base::WeakPtrFactory<FetchRawTransactionsTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_FETCH_RAW_TRANSACTIONS_TASK_H_
