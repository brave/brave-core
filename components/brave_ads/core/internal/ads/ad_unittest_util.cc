/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"

#include "base/guid.h"
#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "url/gurl.h"

namespace brave_ads {

AdInfo BuildAd(const AdType& ad_type, const bool should_use_random_guids) {
  AdInfo ad;

  ad.type = ad_type;

  ad.placement_id = should_use_random_guids
                        ? base::GUID::GenerateRandomV4().AsLowercaseString()
                        : kPlacementId;

  ad.creative_instance_id =
      should_use_random_guids
          ? base::GUID::GenerateRandomV4().AsLowercaseString()
          : kCreativeInstanceId;

  ad.creative_instance_id =
      should_use_random_guids
          ? base::GUID::GenerateRandomV4().AsLowercaseString()
          : kCreativeInstanceId;

  ad.creative_set_id = should_use_random_guids
                           ? base::GUID::GenerateRandomV4().AsLowercaseString()
                           : kCreativeSetId;

  ad.campaign_id = should_use_random_guids
                       ? base::GUID::GenerateRandomV4().AsLowercaseString()
                       : kCampaignId;

  ad.advertiser_id = should_use_random_guids
                         ? base::GUID::GenerateRandomV4().AsLowercaseString()
                         : kAdvertiserId;

  ad.segment = kSegment;

  ad.target_url = GURL("https://brave.com");

  return ad;
}

}  // namespace brave_ads
