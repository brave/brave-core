/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "url/gurl.h"

namespace brave_ads::test {

std::string RandomUuidOr(const bool should_generate_random_uuid,
                         const std::string& or_uuid) {
  return should_generate_random_uuid
             ? base::Uuid::GenerateRandomV4().AsLowercaseString()
             : or_uuid;
}

AdInfo BuildAd(const mojom::AdType mojom_ad_type,
               const bool should_generate_random_uuids) {
  AdInfo ad;

  ad.type = mojom_ad_type;
  ad.placement_id = RandomUuidOr(should_generate_random_uuids, kPlacementId);
  ad.creative_instance_id =
      RandomUuidOr(should_generate_random_uuids, kCreativeInstanceId);
  ad.creative_set_id =
      RandomUuidOr(should_generate_random_uuids, kCreativeSetId);
  ad.campaign_id = RandomUuidOr(should_generate_random_uuids, kCampaignId);
  ad.advertiser_id = RandomUuidOr(should_generate_random_uuids, kAdvertiserId);
  ad.segment = kSegment;
  ad.target_url = GURL(kTargetUrl);

  return ad;
}

}  // namespace brave_ads::test
