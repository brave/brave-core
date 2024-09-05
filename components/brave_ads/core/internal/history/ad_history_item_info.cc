/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads {

AdHistoryItemInfo::AdHistoryItemInfo() = default;

AdHistoryItemInfo::AdHistoryItemInfo(const AdHistoryItemInfo& other) = default;

AdHistoryItemInfo& AdHistoryItemInfo::operator=(
    const AdHistoryItemInfo& other) = default;

AdHistoryItemInfo::AdHistoryItemInfo(AdHistoryItemInfo&& other) noexcept =
    default;

AdHistoryItemInfo& AdHistoryItemInfo::operator=(
    AdHistoryItemInfo&& other) noexcept = default;

AdHistoryItemInfo::~AdHistoryItemInfo() = default;

bool AdHistoryItemInfo::IsValid() const {
  return type != AdType::kUndefined &&
         confirmation_type != ConfirmationType::kUndefined &&
         !placement_id.empty() && !creative_instance_id.empty() &&
         !creative_set_id.empty() && !campaign_id.empty() &&
         !advertiser_id.empty() && !segment.empty() && !title.empty() &&
         !description.empty() && target_url.is_valid();
}

}  // namespace brave_ads
