/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"

#include <utility>

#include "base/bind.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

namespace {

constexpr base::TimeDelta kExpiredTimeDelta =
    base::Seconds(kBlockTrackerDefaultTimeInSeconds);

}

FilBlockTracker::FilBlockTracker(JsonRpcService* json_rpc_service)
    : BlockTracker(json_rpc_service), weak_ptr_factory_(this) {}

FilBlockTracker::~FilBlockTracker() = default;

void FilBlockTracker::Start(base::TimeDelta interval) {
  timer_.Start(FROM_HERE, interval,
               base::BindRepeating(&FilBlockTracker::GetLatestBlockhash,
                                   weak_ptr_factory_.GetWeakPtr(),
                                   base::NullCallback(), false));
}

void FilBlockTracker::GetLatestBlockhash(GetLatestBlockhashCallback callback,
                                         bool try_cached_value) {
  if (try_cached_value && !latest_blockhash_.empty() &&
      latest_blockhash_expired_time_ > base::Time::Now()) {
    std::move(callback).Run(latest_blockhash_,
                            mojom::FilecoinProviderError::kSuccess, "");
    return;
  }

  json_rpc_service_->GetFilChainHead(
      base::BindOnce(&FilBlockTracker::OnGetLatestBlockhash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void FilBlockTracker::OnGetLatestBlockhash(GetLatestBlockhashCallback callback,
                                           const std::string& latest_blockhash,
                                           mojom::FilecoinProviderError error,
                                           const std::string& error_message) {
  if (callback)
    std::move(callback).Run(latest_blockhash, error, error_message);

  if (error != mojom::FilecoinProviderError::kSuccess) {
    VLOG(1) << __FUNCTION__ << ": Failed to get latest blockhash, error: "
            << static_cast<int>(error) << ", error_message: " << error_message;
    return;
  }

  if (latest_blockhash_ == latest_blockhash)
    return;

  latest_blockhash_ = latest_blockhash;
  latest_blockhash_expired_time_ = base::Time::Now() + kExpiredTimeDelta;
  for (auto& observer : observers_)
    observer.OnLatestBlockhashUpdated(latest_blockhash_);
}

void FilBlockTracker::AddObserver(FilBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void FilBlockTracker::RemoveObserver(FilBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
