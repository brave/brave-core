/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_factory.h"

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_base.h"
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_v1.h"
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_v2.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

namespace ads {
namespace inline_content_ads {

std::unique_ptr<EligibleAdsBase> EligibleAdsFactory::Build(
    const int version,
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource) {
  switch (version) {
    case 1: {
      return std::make_unique<EligibleAdsV1>(subdivision_targeting,
                                             anti_targeting_resource);
    }

    case 2: {
      return std::make_unique<EligibleAdsV2>(subdivision_targeting,
                                             anti_targeting_resource);
    }

    default: {
      return nullptr;
    }
  }
}

}  // namespace inline_content_ads
}  // namespace ads
