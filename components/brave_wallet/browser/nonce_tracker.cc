/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/nonce_tracker.h"

#include <algorithm>

#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

NonceTracker::NonceTracker(TxStateManager* tx_state_manager,
                           JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service),
      tx_state_manager_(tx_state_manager) {}

NonceTracker::~NonceTracker() = default;

uint256_t NonceTracker::GetFinalNonce(const std::string& chain_id,
                                      const mojom::AccountIdPtr& from,
                                      uint256_t network_nonce) {
  auto confirmed_transactions = tx_state_manager_->GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Confirmed, from);
  uint256_t local_highest = GetHighestLocallyConfirmed(confirmed_transactions);

  uint256_t highest_confirmed = std::max(network_nonce, local_highest);

  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Submitted, from);

  uint256_t highest_continuous_from =
      GetHighestContinuousFrom(pending_transactions, highest_confirmed);

  uint256_t nonce = std::max(network_nonce, highest_continuous_from);

  return nonce;
}

}  // namespace brave_wallet
