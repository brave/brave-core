/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_block_tracker.h"

#include "base/bind.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

SolanaBlockTracker::SolanaBlockTracker(JsonRpcService* json_rpc_service)
    : BlockTracker(json_rpc_service), weak_ptr_factory_(this) {}

SolanaBlockTracker::~SolanaBlockTracker() = default;

void SolanaBlockTracker::Start(base::TimeDelta interval) {
  latest_blockhash_.clear();
  timer_.Start(FROM_HERE, interval,
               base::BindRepeating(&SolanaBlockTracker::GetLatestBlockhash,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void SolanaBlockTracker::Stop() {
  BlockTracker::Stop();
  latest_blockhash_.clear();
}

void SolanaBlockTracker::GetLatestBlockhash() {
  json_rpc_service_->GetSolanaLatestBlockhash(
      base::BindOnce(&SolanaBlockTracker::OnGetLatestBlockhash,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SolanaBlockTracker::OnGetLatestBlockhash(
    const std::string& latest_blockhash,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess) {
    VLOG(1) << __FUNCTION__ << ": Failed to get latest blockhash, error: "
            << static_cast<int>(error) << ", error_message: " << error_message;
    return;
  }

  if (latest_blockhash_ == latest_blockhash)
    return;

  latest_blockhash_ = latest_blockhash;
  for (auto& observer : observers_)
    observer.OnLatestBlockhashUpdated(latest_blockhash);
}

void SolanaBlockTracker::AddObserver(SolanaBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void SolanaBlockTracker::RemoveObserver(
    SolanaBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
