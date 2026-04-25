/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_block_tracker.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

namespace brave_wallet {

CardanoBlockTracker::CardanoBlockTracker(
    CardanoWalletService& cardano_wallet_service)
    : cardano_wallet_service_(cardano_wallet_service) {}

CardanoBlockTracker::~CardanoBlockTracker() = default;

void CardanoBlockTracker::Start(const std::string& chain_id,
                                base::TimeDelta interval) {
  auto& timer = timers_[chain_id];
  if (!timer) {
    timer = std::make_unique<base::RepeatingTimer>();
  }

  timer->Start(FROM_HERE, interval,
               base::BindRepeating(&CardanoBlockTracker::RequestLatestBlock,
                                   weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void CardanoBlockTracker::RequestLatestBlock(const std::string& chain_id) {
  cardano_wallet_service_->GetCardanoRpc(chain_id)->GetLatestBlock(
      base::BindOnce(&CardanoBlockTracker::OnGetLatestBlock,
                     weak_ptr_factory_.GetWeakPtr(), chain_id));
}

std::optional<uint32_t> CardanoBlockTracker::GetLatestHeight(
    const std::string& chain_id) const {
  auto* height = base::FindOrNull(latest_height_map_, chain_id);
  if (!height) {
    return std::nullopt;
  }
  return *height;
}

void CardanoBlockTracker::OnGetLatestBlock(
    const std::string& chain_id,
    base::expected<cardano_rpc::Block, std::string> latest_block) {
  if (!latest_block.has_value()) {
    return;
  }
  auto cur_latest_height = GetLatestHeight(chain_id);
  if (cur_latest_height && cur_latest_height.value() == latest_block->height) {
    return;
  }
  latest_height_map_[chain_id] = latest_block->height;
  for (auto& observer : observers_) {
    observer.OnLatestHeightUpdated(chain_id, latest_block->height);
  }
}

void CardanoBlockTracker::AddObserver(CardanoBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void CardanoBlockTracker::RemoveObserver(
    CardanoBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
