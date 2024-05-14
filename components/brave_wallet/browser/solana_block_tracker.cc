/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_block_tracker.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

namespace {

constexpr base::TimeDelta kExpiredTimeDelta =
    base::Seconds(kSolanaBlockTrackerTimeInSeconds);

}  // namespace

SolanaBlockTracker::SolanaBlockTracker(JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service), weak_ptr_factory_(this) {}

SolanaBlockTracker::~SolanaBlockTracker() = default;

void SolanaBlockTracker::Start(const std::string& chain_id,
                               base::TimeDelta interval) {
  if (!base::Contains(timers_, chain_id)) {
    timers_[chain_id] = std::make_unique<base::RepeatingTimer>();
  }
  timers_[chain_id]->Start(
      FROM_HERE, interval,
      base::BindRepeating(&SolanaBlockTracker::GetLatestBlockhash,
                          weak_ptr_factory_.GetWeakPtr(), chain_id,
                          base::NullCallback(), false));
}

void SolanaBlockTracker::GetLatestBlockhash(const std::string& chain_id,
                                            GetLatestBlockhashCallback callback,
                                            bool try_cached_value) {
  if (try_cached_value && base::Contains(latest_blockhash_map_, chain_id) &&
      !latest_blockhash_map_[chain_id].empty() &&
      base::Contains(latest_blockhash_expired_time_map_, chain_id) &&
      latest_blockhash_expired_time_map_[chain_id] > base::Time::Now() &&
      base::Contains(last_valid_block_height_map_, chain_id)) {
    std::move(callback).Run(latest_blockhash_map_[chain_id],
                            last_valid_block_height_map_[chain_id],
                            mojom::SolanaProviderError::kSuccess, "");
    return;
  }

  json_rpc_service_->GetSolanaLatestBlockhash(
      chain_id, base::BindOnce(&SolanaBlockTracker::OnGetLatestBlockhash,
                               weak_ptr_factory_.GetWeakPtr(), chain_id,
                               std::move(callback)));
}

void SolanaBlockTracker::OnGetLatestBlockhash(
    const std::string& chain_id,
    GetLatestBlockhashCallback callback,
    const std::string& latest_blockhash,
    uint64_t last_valid_block_height,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (callback) {
    std::move(callback).Run(latest_blockhash, last_valid_block_height, error,
                            error_message);
  }

  if (error != mojom::SolanaProviderError::kSuccess) {
    VLOG(1) << __FUNCTION__ << ": Failed to get latest blockhash, error: "
            << static_cast<int>(error) << ", error_message: " << error_message;
    return;
  }

  if (base::Contains(latest_blockhash_map_, chain_id) &&
      latest_blockhash_map_[chain_id] == latest_blockhash) {
    return;
  }

  latest_blockhash_map_[chain_id] = latest_blockhash;
  last_valid_block_height_map_[chain_id] = last_valid_block_height;
  latest_blockhash_expired_time_map_[chain_id] =
      base::Time::Now() + kExpiredTimeDelta;
  for (auto& observer : observers_) {
    observer.OnLatestBlockhashUpdated(chain_id, latest_blockhash,
                                      last_valid_block_height);
  }
}

void SolanaBlockTracker::AddObserver(SolanaBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void SolanaBlockTracker::RemoveObserver(
    SolanaBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
