/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_builder.h"

#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep

namespace brave_ads {

namespace {
constexpr char kSegment[] = "keyword";
}  // namespace

SearchResultAdInfo BuildSearchResultAd(
    const mojom::SearchResultAdInfoPtr& ad_mojom) {
  SearchResultAdInfo ad;

  ad.type = AdType::kSearchResultAd;
  ad.placement_id = ad_mojom->placement_id;
  ad.creative_instance_id = ad_mojom->creative_instance_id;
  ad.creative_set_id = ad_mojom->creative_set_id;
  ad.campaign_id = ad_mojom->campaign_id;
  ad.advertiser_id = ad_mojom->advertiser_id;
  ad.segment = kSegment;
  ad.target_url = ad_mojom->target_url;
  ad.headline_text = ad_mojom->headline_text;
  ad.description = ad_mojom->description;

  return ad;
}

}  // namespace brave_ads
