/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"

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
  return ad_type == other.ad_type &&
         creative_instance_id == other.creative_instance_id &&
         creative_set_id == other.creative_set_id &&
         campaign_id == other.campaign_id &&
         advertiser_id == other.advertiser_id && segment == other.segment &&
         conversion_id == other.conversion_id &&
         advertiser_public_key == other.advertiser_public_key &&
         process_at == other.process_at && was_processed == other.was_processed;
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
