/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "url/gurl.h"

namespace brave_ads::test {

std::string GetConstantId(const bool should_use_random_uuids,
                          const std::string& constant_id) {
  return should_use_random_uuids
             ? base::Uuid::GenerateRandomV4().AsLowercaseString()
             : constant_id;
}

AdInfo BuildAd(const AdType ad_type, const bool should_use_random_uuids) {
  AdInfo ad;

  ad.type = ad_type;
  ad.placement_id = GetConstantId(should_use_random_uuids, kPlacementId);
  ad.creative_instance_id =
      GetConstantId(should_use_random_uuids, kCreativeInstanceId);
  ad.creative_set_id = GetConstantId(should_use_random_uuids, kCreativeSetId);
  ad.campaign_id = GetConstantId(should_use_random_uuids, kCampaignId);
  ad.advertiser_id = GetConstantId(should_use_random_uuids, kAdvertiserId);
  ad.segment = kSegment;
  ad.target_url = GURL("https://brave.com");

  return ad;
}

}  // namespace brave_ads::test
