/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/new_tab_page_ad_value_util.h"

#include <string>
#include <utility>

#include "brave/components/brave_ads/core/new_tab_page_ad_constants.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"

namespace brave_ads {

namespace {
constexpr char kTypeKey[] = "type";
}  // namespace

base::Value::Dict NewTabPageAdToValue(const NewTabPageAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kNewTabPageAdPlacementIdKey, ad.placement_id);
  dict.Set(kNewTabPageAdCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kNewTabPageAdCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kNewTabPageAdCampaignIdKey, ad.campaign_id);
  dict.Set(kNewTabPageAdAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kNewTabPageAdSegmentKey, ad.segment);
  dict.Set(kNewTabPageAdCompanyNameKey, ad.company_name);
  dict.Set(kNewTabPageAdImageUrlKey, ad.image_url.spec());
  dict.Set(kNewTabPageAdAltKey, ad.alt);
  dict.Set(kNewTabPageAdTargetUrlKey, ad.target_url.spec());

  base::Value::List wallpapers;
  for (const NewTabPageAdWallpaperInfo& wallpaper : ad.wallpapers) {
    base::Value::Dict wallpaper_dict;
    wallpaper_dict.Set(kNewTabPageAdImageUrlKey, wallpaper.image_url.spec());

    base::Value::Dict focal_point;
    focal_point.Set(kNewTabPageAdFocalPointXKey, wallpaper.focal_point.x);
    focal_point.Set(kNewTabPageAdFocalPointYKey, wallpaper.focal_point.y);
    wallpaper_dict.Set(kNewTabPageAdFocalPointKey, std::move(focal_point));

    wallpapers.Append(std::move(wallpaper_dict));
  }
  dict.Set(kNewTabPageAdWallpapersKey, std::move(wallpapers));

  return dict;
}

NewTabPageAdInfo NewTabPageAdFromValue(const base::Value::Dict& root) {
  NewTabPageAdInfo ad;

  if (const auto* value = root.FindString(kTypeKey)) {
    ad.type = AdType(*value);
  }

  if (const auto* value = root.FindString(kNewTabPageAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* value = root.FindString(kNewTabPageAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* value = root.FindString(kNewTabPageAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* value = root.FindString(kNewTabPageAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* value = root.FindString(kNewTabPageAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* value = root.FindString(kNewTabPageAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* value = root.FindString(kNewTabPageAdCompanyNameKey)) {
    ad.company_name = *value;
  }

  if (const auto* value = root.FindString(kNewTabPageAdImageUrlKey)) {
    ad.image_url = GURL(*value);
  }

  if (const auto* value = root.FindString(kNewTabPageAdAltKey)) {
    ad.alt = *value;
  }

  if (const auto* wallpapers = root.FindList(kNewTabPageAdWallpapersKey)) {
    for (const auto& value : *wallpapers) {
      const base::Value::Dict* const dict = value.GetIfDict();
      if (!dict) {
        continue;
      }

      const std::string* const image_url =
          dict->FindString(kNewTabPageAdImageUrlKey);
      const base::Value::Dict* const focal_point =
          dict->FindDict(kNewTabPageAdFocalPointKey);
      if (!image_url || !focal_point) {
        continue;
      }

      const absl::optional<int> x =
          focal_point->FindInt(kNewTabPageAdFocalPointXKey);
      const absl::optional<int> y =
          focal_point->FindInt(kNewTabPageAdFocalPointYKey);
      if (!x || !y) {
        continue;
      }

      NewTabPageAdWallpaperInfo wallpaper;
      wallpaper.image_url = GURL(*image_url);
      wallpaper.focal_point.x = *x;
      wallpaper.focal_point.y = *y;

      ad.wallpapers.push_back(wallpaper);
    }
  }

  if (const auto* value = root.FindString(kNewTabPageAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace brave_ads
