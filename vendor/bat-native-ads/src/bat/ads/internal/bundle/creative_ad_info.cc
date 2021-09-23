/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/container_util.h"

namespace ads {

CreativeAdInfo::CreativeAdInfo() = default;

CreativeAdInfo::CreativeAdInfo(const CreativeAdInfo& info) = default;

CreativeAdInfo::~CreativeAdInfo() = default;

bool CreativeAdInfo::operator==(const CreativeAdInfo& rhs) const {
  return creative_instance_id == rhs.creative_instance_id &&
         creative_set_id == rhs.creative_set_id &&
         campaign_id == rhs.campaign_id &&
         start_at_timestamp == rhs.start_at_timestamp &&
         end_at_timestamp == rhs.end_at_timestamp &&
         daily_cap == rhs.daily_cap && advertiser_id == rhs.advertiser_id &&
         priority == rhs.priority && ptr == rhs.ptr &&
         conversion == rhs.conversion && per_day == rhs.per_day &&
         per_week == rhs.per_week && per_month == rhs.per_month &&
         total_max == rhs.total_max &&
         split_test_group == rhs.split_test_group && segment == rhs.segment &&
         geo_targets == rhs.geo_targets && target_url == rhs.target_url &&
         dayparts == rhs.dayparts;
}

bool CreativeAdInfo::operator!=(const CreativeAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
