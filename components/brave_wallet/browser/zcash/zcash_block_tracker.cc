/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_block_tracker.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"

namespace brave_wallet {

ZCashBlockTracker::ZCashBlockTracker(ZCashRpc& zcash_rpc)
    : zcash_rpc_(zcash_rpc) {}

ZCashBlockTracker::~ZCashBlockTracker() = default;

void ZCashBlockTracker::Start(const std::string& chain_id,
                              base::TimeDelta interval) {
  if (!base::Contains(timers_, chain_id)) {
    timers_[chain_id] = std::make_unique<base::RepeatingTimer>();
  }

  timers_[chain_id]->Start(
      FROM_HERE, interval,
      base::BindRepeating(&ZCashBlockTracker::GetBlockHeight,
                          weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void ZCashBlockTracker::GetBlockHeight(const std::string& chain_id) {
  zcash_rpc_->GetLatestBlock(
      chain_id, base::BindOnce(&ZCashBlockTracker::OnGetLatestBlockForHeight,
                               weak_ptr_factory_.GetWeakPtr(), chain_id));
}

std::optional<uint32_t> ZCashBlockTracker::GetLatestHeight(
    const std::string& chain_id) const {
  if (!base::Contains(latest_height_map_, chain_id)) {
    return std::nullopt;
  }
  return latest_height_map_.at(chain_id);
}

void ZCashBlockTracker::OnGetLatestBlockForHeight(
    const std::string& chain_id,
    base::expected<zcash::mojom::BlockIDPtr, std::string> latest_block) {
  if (!latest_block.has_value() || !latest_block.value()) {
    return;
  }
  auto cur_latest_height = GetLatestHeight(chain_id);
  if (cur_latest_height &&
      cur_latest_height.value() == (*latest_block)->height) {
    return;
  }
  latest_height_map_[chain_id] = (*latest_block)->height;
  for (auto& observer : observers_) {
    observer.OnLatestHeightUpdated(chain_id, (*latest_block)->height);
  }
}

void ZCashBlockTracker::AddObserver(ZCashBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void ZCashBlockTracker::RemoveObserver(ZCashBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
