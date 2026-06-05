/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

ConfirmationQueueItemInfo::ConfirmationQueueItemInfo() = default;

ConfirmationQueueItemInfo::ConfirmationQueueItemInfo(
    const ConfirmationQueueItemInfo& other) = default;

ConfirmationQueueItemInfo& ConfirmationQueueItemInfo::operator=(
    const ConfirmationQueueItemInfo& other) = default;

ConfirmationQueueItemInfo::ConfirmationQueueItemInfo(
    ConfirmationQueueItemInfo&& other) noexcept = default;

ConfirmationQueueItemInfo& ConfirmationQueueItemInfo::operator=(
    ConfirmationQueueItemInfo&& other) noexcept = default;

ConfirmationQueueItemInfo::~ConfirmationQueueItemInfo() = default;

bool ConfirmationQueueItemInfo::IsValid() const {
  // All four confirmation fields map to NOT NULL TEXT DB columns, so legacy
  // rows satisfy these constraints. A type/ad_type string that no longer maps
  // to a known enum yields kUndefined and is silently dropped on the read
  // path — it is never CHECKed.
  return process_at && !confirmation.transaction_id.empty() &&
         !confirmation.creative_instance_id.empty() &&
         confirmation.type != mojom::ConfirmationType::kUndefined &&
         confirmation.ad_type != mojom::AdType::kUndefined;
}

}  // namespace brave_ads
