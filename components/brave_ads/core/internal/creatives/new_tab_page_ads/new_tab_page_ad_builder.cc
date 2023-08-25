/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads/new_tab_page_ad_info.h"

namespace brave_ads {

NewTabPageAdInfo BuildNewTabPageAd(
    const CreativeNewTabPageAdInfo& creative_ad) {
  const std::string placement_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  return BuildNewTabPageAd(creative_ad, placement_id);
}

NewTabPageAdInfo BuildNewTabPageAd(const CreativeNewTabPageAdInfo& creative_ad,
                                   const std::string& placement_id) {
  NewTabPageAdInfo ad;

  ad.type = AdType::kNewTabPageAd;
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

  for (const auto& creative_ad_wallpaper : creative_ad.wallpapers) {
    NewTabPageAdWallpaperInfo wallpaper;

    wallpaper.image_url = creative_ad_wallpaper.image_url;

    NewTabPageAdWallpaperFocalPointInfo focal_point;
    focal_point.x = creative_ad_wallpaper.focal_point.x;
    focal_point.y = creative_ad_wallpaper.focal_point.y;
    wallpaper.focal_point = focal_point;

    ad.wallpapers.push_back(wallpaper);
  }

  return ad;
}

}  // namespace brave_ads
