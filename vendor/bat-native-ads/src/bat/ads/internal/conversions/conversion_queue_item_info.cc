/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversion_queue_item_info.h"

#include "bat/ads/internal/number_util.h"

namespace ads {

ConversionQueueItemInfo::ConversionQueueItemInfo() = default;

ConversionQueueItemInfo::ConversionQueueItemInfo(
    const ConversionQueueItemInfo& info) = default;

ConversionQueueItemInfo::~ConversionQueueItemInfo() = default;

bool ConversionQueueItemInfo::operator==(
    const ConversionQueueItemInfo& rhs) const {
  return campaign_id == rhs.campaign_id &&
         creative_set_id == rhs.creative_set_id &&
         creative_instance_id == rhs.creative_instance_id &&
         advertiser_id == rhs.advertiser_id &&
         conversion_id == rhs.conversion_id &&
         advertiser_public_key == rhs.advertiser_public_key &&
         DoubleEquals(confirm_at.ToDoubleT(), rhs.confirm_at.ToDoubleT());
}

bool ConversionQueueItemInfo::operator!=(
    const ConversionQueueItemInfo& rhs) const {
  return !(*this == rhs);
}

bool ConversionQueueItemInfo::IsValid() const {
  // campaign_id and advertiser_id will be empty for legacy conversions migrated
  // from |ad_conversions.json| to |database.sqlite| and conversion_id will be
  // empty for non verifiable conversions
  if (creative_set_id.empty() || creative_instance_id.empty() ||
      confirm_at.is_null()) {
    return false;
  }

  return true;
}

}  // namespace ads
