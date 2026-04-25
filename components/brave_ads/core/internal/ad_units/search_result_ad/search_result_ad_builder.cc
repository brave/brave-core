/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_builder.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {
constexpr char kSegment[] = "keyword";
}  // namespace

SearchResultAdInfo FromMojomBuildSearchResultAd(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad) {
  CHECK(mojom_creative_ad);

  SearchResultAdInfo ad;

  ad.type = mojom::AdType::kSearchResultAd;
  ad.placement_id = mojom_creative_ad->placement_id;
  ad.creative_instance_id = mojom_creative_ad->creative_instance_id;
  ad.creative_set_id = mojom_creative_ad->creative_set_id;
  ad.campaign_id = mojom_creative_ad->campaign_id;
  ad.advertiser_id = mojom_creative_ad->advertiser_id;
  ad.segment = kSegment;
  ad.target_url = mojom_creative_ad->target_url;
  ad.headline_text = mojom_creative_ad->headline_text;
  ad.description = mojom_creative_ad->description;

  return ad;
}

}  // namespace brave_ads
