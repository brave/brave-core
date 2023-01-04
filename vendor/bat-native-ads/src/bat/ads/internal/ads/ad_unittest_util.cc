/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_unittest_util.h"

#include "bat/ads/ad_info.h"
#include "url/gurl.h"

namespace ads {

namespace {

constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";
constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
constexpr char kCreativeSetId[] = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
constexpr char kCampaignId[] = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
constexpr char kAdvertiserId[] = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
constexpr char kSegment[] = "segment";

}  // namespace

AdInfo BuildAd() {
  AdInfo ad;

  ad.type = AdType::kNotificationAd;
  ad.placement_id = kPlacementId;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.creative_set_id = kCreativeSetId;
  ad.campaign_id = kCampaignId;
  ad.advertiser_id = kAdvertiserId;
  ad.segment = kSegment;
  ad.target_url = GURL("https://brave.com");

  return ad;
}

}  // namespace ads
