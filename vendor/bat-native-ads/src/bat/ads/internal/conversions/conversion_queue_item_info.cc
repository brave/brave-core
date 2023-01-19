/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversion_queue_item_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads {

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
  return campaign_id == other.campaign_id &&
         creative_set_id == other.creative_set_id &&
         creative_instance_id == other.creative_instance_id &&
         advertiser_id == other.advertiser_id &&
         conversion_id == other.conversion_id &&
         advertiser_public_key == other.advertiser_public_key &&
         ad_type == other.ad_type &&
         DoubleEquals(process_at.ToDoubleT(), other.process_at.ToDoubleT()) &&
         was_processed == other.was_processed;
}

bool ConversionQueueItemInfo::operator!=(
    const ConversionQueueItemInfo& other) const {
  return !(*this == other);
}

bool ConversionQueueItemInfo::IsValid() const {
  // campaign_id and advertiser_id will be empty for legacy conversions migrated
  // from |ad_conversions.json| to |database.sqlite| and conversion_id will be
  // empty for non verifiable conversions
  return !(creative_set_id.empty() || creative_instance_id.empty() ||
           process_at.is_null());
}

}  // namespace ads
