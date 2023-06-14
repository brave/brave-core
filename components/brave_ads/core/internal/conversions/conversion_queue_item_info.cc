/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"

#include <tuple>

namespace brave_ads {

ConversionQueueItemInfo::ConversionQueueItemInfo() = default;

ConversionQueueItemInfo::ConversionQueueItemInfo(
    const ConversionQueueItemInfo& other) = default;

ConversionQueueItemInfo& ConversionQueueItemInfo::operator=(
    const ConversionQueueItemInfo& other) = default;

ConversionQueueItemInfo::ConversionQueueItemInfo(
    ConversionQueueItemInfo&& other) noexcept = default;

ConversionQueueItemInfo& ConversionQueueItemInfo::operator=(
    ConversionQueueItemInfo&& other) noexcept = default;

ConversionQueueItemInfo::~ConversionQueueItemInfo() = default;

bool ConversionQueueItemInfo::operator==(
    const ConversionQueueItemInfo& other) const {
  const auto tie = [](const ConversionQueueItemInfo& conversion_queue_item) {
    return std::tie(
        conversion_queue_item.ad_type,
        conversion_queue_item.creative_instance_id,
        conversion_queue_item.creative_set_id,
        conversion_queue_item.campaign_id, conversion_queue_item.advertiser_id,
        conversion_queue_item.segment, conversion_queue_item.conversion_id,
        conversion_queue_item.advertiser_public_key,
        conversion_queue_item.process_at, conversion_queue_item.was_processed);
  };

  return tie(*this) == tie(other);
}

bool ConversionQueueItemInfo::operator!=(
    const ConversionQueueItemInfo& other) const {
  return !(*this == other);
}

bool ConversionQueueItemInfo::IsValid() const {
  // campaign_id and advertiser_id will be empty for legacy conversions migrated
  // from |ad_conversions.json| to |database.sqlite| and conversion_id will be
  // empty for non verifiable conversions
  return !creative_instance_id.empty() && !creative_set_id.empty() &&
         !process_at.is_null();
}

}  // namespace brave_ads
