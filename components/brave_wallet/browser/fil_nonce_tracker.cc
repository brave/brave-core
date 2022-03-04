/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/check_op.h"
#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/nonce_tracker.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/fil_address.h"

namespace brave_wallet {

FilNonceTracker::FilNonceTracker(TxStateManager* tx_state_manager,
                                 JsonRpcService* json_rpc_service)
    : NonceTracker(tx_state_manager, json_rpc_service), weak_factory_(this) {}
FilNonceTracker::~FilNonceTracker() = default;
void FilNonceTracker::GetNextNonce(const std::string& from,
                                   GetNextNonceCallback callback) {
  GetJsonRpcService()->GetFilTransactionCount(
      from,
      base::BindOnce(&NonceTracker::OnGetNetworkNonce,
                     weak_factory_.GetWeakPtr(), from, std::move(callback)));
}

std::string FilNonceTracker::GetChecksumAddress(const std::string& address) {
  return FilAddress::FromAddress(address).ToChecksumAddress();
}

uint256_t FilNonceTracker::GetHighestLocallyConfirmed(
    const std::vector<std::unique_ptr<TxMeta>>& metas) {
  uint64_t highest = 0;
  for (auto& meta : metas) {
    auto* tx_meta = static_cast<FilTxMeta*>(meta.get());
    DCHECK(
        tx_meta->tx()->nonce());  // Not supposed to happen for a confirmed tx.
    highest = std::max(highest, tx_meta->tx()->nonce().value() + (uint64_t)1);
  }
  return static_cast<uint256_t>(highest);
}

uint256_t FilNonceTracker::GetHighestContinuousFrom(
    const std::vector<std::unique_ptr<TxMeta>>& metas,
    uint256_t start) {
  bool valid_range = start <= static_cast<uint256_t>(UINT64_MAX);
  DCHECK(valid_range);
  uint64_t highest = start;
  for (auto& meta : metas) {
    auto* eth_meta = static_cast<FilTxMeta*>(meta.get());
    DCHECK(
        eth_meta->tx()->nonce());  // Not supposed to happen for a submitted tx.
    if (eth_meta->tx()->nonce().value() == highest)
      highest++;
  }
  return static_cast<uint256_t>(highest);
}

}  // namespace brave_wallet
