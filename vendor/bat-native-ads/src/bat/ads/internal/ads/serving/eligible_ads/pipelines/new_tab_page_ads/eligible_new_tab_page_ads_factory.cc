/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_factory.h"

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_base.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v1.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace ads::new_tab_page_ads {

std::unique_ptr<EligibleAdsBase> EligibleAdsFactory::Build(
    const int version,
    geographic::SubdivisionTargeting* subdivision_targeting,
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

}  // namespace ads::new_tab_page_ads
