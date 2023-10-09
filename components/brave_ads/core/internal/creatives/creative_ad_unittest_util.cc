/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ad_unittest_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "url/gurl.h"

namespace brave_ads::test {

CreativeAdInfo BuildCreativeAd(const bool should_use_random_uuids) {
  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCreativeInstanceId;

  creative_ad.creative_set_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCreativeSetId;

  creative_ad.campaign_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCampaignId;

  creative_ad.advertiser_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kAdvertiserId;

  creative_ad.start_at = DistantPast();
  creative_ad.end_at = DistantFuture();

  creative_ad.daily_cap = 2;

  creative_ad.priority = 2;

  creative_ad.pass_through_rate = 1.0;

  creative_ad.per_day = 3;
  creative_ad.per_week = 4;
  creative_ad.per_month = 5;

  creative_ad.total_max = 6;

  creative_ad.value = 1.0;

  creative_ad.segment = "untargeted";

  creative_ad.split_test_group = "";

  creative_ad.dayparts = {
      {.days_of_week = "0123456", .start_minute = 0, .end_minute = 1439}};

  creative_ad.geo_targets = {"US"};

  creative_ad.target_url = GURL("https://brave.com");

  return creative_ad;
}

}  // namespace brave_ads::test
