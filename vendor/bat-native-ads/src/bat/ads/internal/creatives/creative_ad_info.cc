/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/creative_ad_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads {

CreativeAdInfo::CreativeAdInfo() = default;

CreativeAdInfo::CreativeAdInfo(const CreativeAdInfo& other) = default;

CreativeAdInfo& CreativeAdInfo::operator=(const CreativeAdInfo& other) =
    default;

CreativeAdInfo::CreativeAdInfo(CreativeAdInfo&& other) noexcept = default;

CreativeAdInfo& CreativeAdInfo::operator=(CreativeAdInfo&& other) noexcept =
    default;

CreativeAdInfo::~CreativeAdInfo() = default;

bool CreativeAdInfo::operator==(const CreativeAdInfo& other) const {
  return creative_instance_id == other.creative_instance_id &&
         creative_set_id == other.creative_set_id &&
         campaign_id == other.campaign_id &&
         DoubleEquals(start_at.ToDoubleT(), other.start_at.ToDoubleT()) &&
         DoubleEquals(end_at.ToDoubleT(), other.end_at.ToDoubleT()) &&
         daily_cap == other.daily_cap && advertiser_id == other.advertiser_id &&
         priority == other.priority && DoubleEquals(ptr, other.ptr) &&
         conversion == other.conversion && per_day == other.per_day &&
         per_week == other.per_week && per_month == other.per_month &&
         total_max == other.total_max && DoubleEquals(value, other.value) &&
         split_test_group == other.split_test_group &&
         segment == other.segment && geo_targets == other.geo_targets &&
         target_url == other.target_url && dayparts == other.dayparts;
}

bool CreativeAdInfo::operator!=(const CreativeAdInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
