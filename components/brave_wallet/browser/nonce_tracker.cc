/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/nonce_tracker.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

NonceTracker::NonceTracker(TxStateManager* tx_state_manager,
                           JsonRpcService* json_rpc_service)
    : tx_state_manager_(tx_state_manager),
      json_rpc_service_(json_rpc_service) {}
NonceTracker::~NonceTracker() = default;

JsonRpcService* NonceTracker::GetJsonRpcService() {
  return json_rpc_service_.get();
}

absl::optional<uint256_t> NonceTracker::GetFinalNonce(const std::string& from,
                                                      uint256_t network_nonce) {
  if (!nonce_lock_.Try()) {
    return absl::nullopt;
  }

  auto confirmed_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Confirmed, from);
  uint256_t local_highest = GetHighestLocallyConfirmed(confirmed_transactions);

  uint256_t highest_confirmed = std::max(network_nonce, local_highest);

  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, from);

  uint256_t highest_continuous_from =
      GetHighestContinuousFrom(pending_transactions, highest_confirmed);

  uint256_t nonce = std::max(network_nonce, highest_continuous_from);

  nonce_lock_.Release();
  return nonce;
}

}  // namespace brave_wallet
