/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

FilBlockTracker::FilBlockTracker(JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service), weak_ptr_factory_(this) {}

FilBlockTracker::~FilBlockTracker() = default;

void FilBlockTracker::Start(const std::string& chain_id,
                            base::TimeDelta interval) {
  if (!base::Contains(timers_, chain_id)) {
    timers_[chain_id] = std::make_unique<base::RepeatingTimer>();
  }
  timers_[chain_id]->Start(
      FROM_HERE, interval,
      base::BindRepeating(&FilBlockTracker::GetFilBlockHeight,
                          weak_ptr_factory_.GetWeakPtr(), chain_id,
                          base::NullCallback()));
}

void FilBlockTracker::GetFilBlockHeight(const std::string& chain_id,
                                        GetFilBlockHeightCallback callback) {
  json_rpc_service_->GetFilBlockHeight(
      chain_id, base::BindOnce(&FilBlockTracker::OnGetFilBlockHeight,
                               weak_ptr_factory_.GetWeakPtr(), chain_id,
                               std::move(callback)));
}

uint64_t FilBlockTracker::GetLatestHeight(const std::string& chain_id) const {
  if (!base::Contains(latest_height_map_, chain_id)) {
    return 0;
  }
  return latest_height_map_.at(chain_id);
}

void FilBlockTracker::OnGetFilBlockHeight(const std::string& chain_id,
                                          GetFilBlockHeightCallback callback,
                                          uint64_t latest_height,
                                          mojom::FilecoinProviderError error,
                                          const std::string& error_message) {
  if (callback) {
    std::move(callback).Run(latest_height, error, error_message);
  }

  if (error != mojom::FilecoinProviderError::kSuccess) {
    VLOG(1) << __FUNCTION__ << ": Failed to get latest height, error: "
            << static_cast<int>(error) << ", error_message: " << error_message;
    return;
  }
  if (GetLatestHeight(chain_id) == latest_height) {
    return;
  }
  latest_height_map_[chain_id] = latest_height;
  for (auto& observer : observers_) {
    observer.OnLatestHeightUpdated(chain_id, latest_height);
  }
}

void FilBlockTracker::AddObserver(FilBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void FilBlockTracker::RemoveObserver(FilBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
