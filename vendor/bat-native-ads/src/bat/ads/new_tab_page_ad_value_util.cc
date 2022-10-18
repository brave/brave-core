/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_value_util.h"

#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {

namespace {

constexpr char kTypeKey[] = "type";
constexpr char kPlacementIdKey[] = "placement_id";
constexpr char kCreativeInstanceIdKey[] = "creative_instance_id";
constexpr char kCreativeSetIdKey[] = "creative_set_id";
constexpr char kCampaignIdKey[] = "campaign_id";
constexpr char kAdvertiserIdKey[] = "advertiser_id";
constexpr char kSegmentKey[] = "segment";
constexpr char kCompanyNameKey[] = "company_name";
constexpr char kAltKey[] = "alt";
constexpr char kImageUrlKey[] = "image_url";
constexpr char kFocalPointKey[] = "focal_point";
constexpr char kFocalPointXKey[] = "x";
constexpr char kFocalPointYKey[] = "y";
constexpr char kWallpapersKey[] = "wallpapers";
constexpr char kTargetUrlKey[] = "target_url";

}  // namespace

base::Value::Dict NewTabPageAdToValue(const NewTabPageAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kPlacementIdKey, ad.placement_id);
  dict.Set(kCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kCampaignIdKey, ad.campaign_id);
  dict.Set(kAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kSegmentKey, ad.segment);
  dict.Set(kCompanyNameKey, ad.company_name);
  dict.Set(kImageUrlKey, ad.image_url.spec());
  dict.Set(kAltKey, ad.alt);
  dict.Set(kTargetUrlKey, ad.target_url.spec());

  base::Value::List wallpapers;
  for (const NewTabPageAdWallpaperInfo& wallpaper : ad.wallpapers) {
    base::Value::Dict wallpaper_dict;
    wallpaper_dict.Set(kImageUrlKey, wallpaper.image_url.spec());

    base::Value::Dict focal_point;
    focal_point.Set(kFocalPointXKey, wallpaper.focal_point.x);
    focal_point.Set(kFocalPointYKey, wallpaper.focal_point.y);
    wallpaper_dict.Set(kFocalPointKey, std::move(focal_point));

    wallpapers.Append(std::move(wallpaper_dict));
  }
  dict.Set(kWallpapersKey, std::move(wallpapers));

  return dict;
}

NewTabPageAdInfo NewTabPageAdFromValue(const base::Value::Dict& root) {
  NewTabPageAdInfo ad;

  if (const auto* value = root.FindString(kTypeKey)) {
    ad.type = AdType(*value);
  }

  if (const auto* value = root.FindString(kPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* value = root.FindString(kCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* value = root.FindString(kCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* value = root.FindString(kCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* value = root.FindString(kAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* value = root.FindString(kSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* value = root.FindString(kCompanyNameKey)) {
    ad.company_name = *value;
  }

  if (const auto* value = root.FindString(kImageUrlKey)) {
    ad.image_url = GURL(*value);
  }

  if (const auto* value = root.FindString(kAltKey)) {
    ad.alt = *value;
  }

  if (const auto* wallpapers = root.FindList(kWallpapersKey)) {
    for (const auto& value : *wallpapers) {
      const base::Value::Dict* dict = value.GetIfDict();
      if (!dict) {
        continue;
      }

      const std::string* image_url = dict->FindString(kImageUrlKey);
      const base::Value::Dict* focal_point = dict->FindDict(kFocalPointKey);
      if (!image_url || !focal_point) {
        continue;
      }

      const absl::optional<int> x = focal_point->FindInt(kFocalPointXKey);
      const absl::optional<int> y = focal_point->FindInt(kFocalPointYKey);
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

  if (const auto* value = root.FindString(kTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace ads
