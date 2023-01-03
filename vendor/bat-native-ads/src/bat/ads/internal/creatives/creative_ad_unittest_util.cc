/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/creative_ad_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "url/gurl.h"

namespace ads {

namespace {

constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
constexpr char kCreativeSetId[] = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
constexpr char kCampaignId[] = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
constexpr char kAdvertiserId[] = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";

}  // namespace

CreativeAdInfo BuildCreativeAd(const bool should_use_random_guids) {
  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id =
      should_use_random_guids
          ? base::GUID::GenerateRandomV4().AsLowercaseString()
          : kCreativeInstanceId;
  creative_ad.creative_set_id =
      should_use_random_guids
          ? base::GUID::GenerateRandomV4().AsLowercaseString()
          : kCreativeSetId;
  creative_ad.campaign_id =
      should_use_random_guids
          ? base::GUID::GenerateRandomV4().AsLowercaseString()
          : kCampaignId;
  creative_ad.start_at = DistantPast();
  creative_ad.end_at = DistantFuture();
  creative_ad.daily_cap = 2;
  creative_ad.advertiser_id =
      should_use_random_guids
          ? base::GUID::GenerateRandomV4().AsLowercaseString()
          : kAdvertiserId;
  creative_ad.priority = 2;
  creative_ad.ptr = 1.0;
  creative_ad.per_day = 3;
  creative_ad.per_week = 4;
  creative_ad.per_month = 5;
  creative_ad.total_max = 6;
  creative_ad.value = 2.0;
  creative_ad.segment = "untargeted";
  creative_ad.split_test_group = "";
  const CreativeDaypartInfo daypart;
  creative_ad.dayparts = {daypart};
  creative_ad.geo_targets = {"US"};
  creative_ad.target_url = GURL("https://brave.com");

  return creative_ad;
}

}  // namespace ads
