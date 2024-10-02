/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

NewTabPageAdInfo BuildNewTabPageAd(
    const CreativeNewTabPageAdInfo& creative_ad) {
  const std::string placement_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  return BuildNewTabPageAd(placement_id, creative_ad);
}

NewTabPageAdInfo BuildNewTabPageAd(
    const std::string& placement_id,
    const CreativeNewTabPageAdInfo& creative_ad) {
  NewTabPageAdInfo ad;

  ad.type = mojom::AdType::kNewTabPageAd;
  ad.placement_id = placement_id;
  ad.creative_instance_id = creative_ad.creative_instance_id;
  ad.creative_set_id = creative_ad.creative_set_id;
  ad.campaign_id = creative_ad.campaign_id;
  ad.advertiser_id = creative_ad.advertiser_id;
  ad.segment = creative_ad.segment;
  ad.company_name = creative_ad.company_name;
  ad.image_url = creative_ad.image_url;
  ad.alt = creative_ad.alt;
  ad.target_url = creative_ad.target_url;

  for (const auto& [image_url, focal_point, _] : creative_ad.wallpapers) {
    ad.wallpapers.push_back(NewTabPageAdWallpaperInfo{
        .image_url = image_url,
        .focal_point =
            NewTabPageAdWallpaperFocalPointInfo{focal_point.x, focal_point.y}});
  }

  return ad;
}

}  // namespace brave_ads
