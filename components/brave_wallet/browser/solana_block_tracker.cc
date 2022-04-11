/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_block_tracker.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

namespace {

constexpr base::TimeDelta kExpiredTimeDelta =
    base::Seconds(kBlockTrackerDefaultTimeInSeconds);

}

SolanaBlockTracker::SolanaBlockTracker(JsonRpcService* json_rpc_service)
    : BlockTracker(json_rpc_service), weak_ptr_factory_(this) {}

SolanaBlockTracker::~SolanaBlockTracker() = default;

void SolanaBlockTracker::Start(base::TimeDelta interval) {
  timer_.Start(FROM_HERE, interval,
               base::BindRepeating(&SolanaBlockTracker::GetLatestBlockhash,
                                   weak_ptr_factory_.GetWeakPtr(),
                                   base::NullCallback(), false));
}

void SolanaBlockTracker::Stop() {
  BlockTracker::Stop();
}

void SolanaBlockTracker::GetLatestBlockhash(GetLatestBlockhashCallback callback,
                                            bool try_cached_value) {
  if (try_cached_value && !latest_blockhash_.empty() &&
      latest_blockhash_expired_time_ > base::Time::Now()) {
    std::move(callback).Run(latest_blockhash_, last_valid_block_height_,
                            mojom::SolanaProviderError::kSuccess, "");
    return;
  }

  json_rpc_service_->GetSolanaLatestBlockhash(
      base::BindOnce(&SolanaBlockTracker::OnGetLatestBlockhash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SolanaBlockTracker::OnGetLatestBlockhash(
    GetLatestBlockhashCallback callback,
    const std::string& latest_blockhash,
    uint64_t last_valid_block_height,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (callback)
    std::move(callback).Run(latest_blockhash, last_valid_block_height, error,
                            error_message);

  if (error != mojom::SolanaProviderError::kSuccess) {
    VLOG(1) << __FUNCTION__ << ": Failed to get latest blockhash, error: "
            << static_cast<int>(error) << ", error_message: " << error_message;
    return;
  }

  if (latest_blockhash_ == latest_blockhash)
    return;

  latest_blockhash_ = latest_blockhash;
  last_valid_block_height_ = last_valid_block_height;
  latest_blockhash_expired_time_ = base::Time::Now() + kExpiredTimeDelta;
  for (auto& observer : observers_)
    observer.OnLatestBlockhashUpdated(latest_blockhash,
                                      last_valid_block_height);
}

void SolanaBlockTracker::AddObserver(SolanaBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void SolanaBlockTracker::RemoveObserver(
    SolanaBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
