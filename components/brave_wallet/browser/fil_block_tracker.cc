/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

FilBlockTracker::FilBlockTracker(JsonRpcService* json_rpc_service)
    : BlockTracker(json_rpc_service), weak_ptr_factory_(this) {}

FilBlockTracker::~FilBlockTracker() = default;

void FilBlockTracker::Start(base::TimeDelta interval) {
  timer_.Start(FROM_HERE, interval,
               base::BindRepeating(&FilBlockTracker::GetFilBlockHeight,
                                   weak_ptr_factory_.GetWeakPtr(),
                                   base::NullCallback()));
}

void FilBlockTracker::GetFilBlockHeight(GetFilBlockHeightCallback callback) {
  json_rpc_service_->GetFilBlockHeight(
      base::BindOnce(&FilBlockTracker::OnGetFilBlockHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void FilBlockTracker::OnGetFilBlockHeight(GetFilBlockHeightCallback callback,
                                          uint64_t latest_height,
                                          mojom::FilecoinProviderError error,
                                          const std::string& error_message) {
  if (callback)
    std::move(callback).Run(latest_height, error, error_message);

  if (error != mojom::FilecoinProviderError::kSuccess) {
    VLOG(1) << __FUNCTION__ << ": Failed to get latest height, error: "
            << static_cast<int>(error) << ", error_message: " << error_message;
    return;
  }
  if (latest_height_ == latest_height)
    return;
  latest_height_ = latest_height;
  for (auto& observer : observers_)
    observer.OnLatestHeightUpdated(latest_height);
}

void FilBlockTracker::AddObserver(FilBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void FilBlockTracker::RemoveObserver(FilBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
