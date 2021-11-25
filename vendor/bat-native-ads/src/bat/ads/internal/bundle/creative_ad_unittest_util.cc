/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_ad_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/unittest_time_util.h"

namespace ads {

CreativeAdInfo BuildCreativeAd() {
  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = base::GenerateGUID();
  creative_ad.creative_set_id = base::GenerateGUID();
  creative_ad.campaign_id = base::GenerateGUID();
  creative_ad.start_at = DistantPast();
  creative_ad.end_at = DistantFuture();
  creative_ad.daily_cap = 1;
  creative_ad.advertiser_id = base::GenerateGUID();
  creative_ad.priority = 2;
  creative_ad.ptr = 1.0;
  creative_ad.per_day = 3;
  creative_ad.per_week = 4;
  creative_ad.per_month = 5;
  creative_ad.total_max = 6;
  creative_ad.value = 2.0;
  creative_ad.segment = "untargeted";
  creative_ad.split_test_group = "";
  CreativeDaypartInfo daypart;
  creative_ad.dayparts = {daypart};
  creative_ad.geo_targets = {"US"};
  creative_ad.target_url = "https://brave.com";

  return creative_ad;
}

}  // namespace ads
