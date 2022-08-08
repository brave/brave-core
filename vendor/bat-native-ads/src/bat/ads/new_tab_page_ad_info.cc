/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_info.h"

#include <tuple>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

namespace ads {

NewTabPageAdInfo::NewTabPageAdInfo() = default;

NewTabPageAdInfo::NewTabPageAdInfo(const NewTabPageAdInfo& info) = default;

NewTabPageAdInfo& NewTabPageAdInfo::operator=(const NewTabPageAdInfo& info) =
    default;

NewTabPageAdInfo::~NewTabPageAdInfo() = default;

bool NewTabPageAdInfo::operator==(const NewTabPageAdInfo& rhs) const {
  if (!AdInfo::operator==(rhs)) {
    return false;
  }

  auto tie = [](const NewTabPageAdInfo& ad) {
    return std::tie(ad.company_name, ad.image_url, ad.alt, ad.wallpapers);
  };

  return tie(*this) == tie(rhs);
}

bool NewTabPageAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (company_name.empty() || !image_url.is_valid() || alt.empty() ||
      wallpapers.empty()) {
    return false;
  }

  return true;
}

base::Value::Dict NewTabPageAdInfo::ToValue() const {
  base::Value::Dict dict;

  dict.Set("type", type.ToString());
  dict.Set("placement_id", placement_id);
  dict.Set("creative_instance_id", creative_instance_id);
  dict.Set("creative_set_id", creative_set_id);
  dict.Set("campaign_id", campaign_id);
  dict.Set("advertiser_id", advertiser_id);
  dict.Set("segment", segment);
  dict.Set("company_name", company_name);
  dict.Set("image_url", image_url.spec());
  dict.Set("alt", alt);
  dict.Set("target_url", target_url.spec());

  base::Value::List wallpapers_list;
  for (const NewTabPageAdWallpaperInfo& wallpaper_info : wallpapers) {
    base::Value::Dict wallpaper;
    wallpaper.Set("image_url", wallpaper_info.image_url.spec());

    base::Value::Dict focal_point;
    focal_point.Set("x", wallpaper_info.focal_point.x);
    focal_point.Set("y", wallpaper_info.focal_point.y);
    wallpaper.Set("focal_point", std::move(focal_point));

    wallpapers_list.Append(std::move(wallpaper));
  }
  dict.Set("wallpapers", std::move(wallpapers_list));
  return dict;
}

void NewTabPageAdInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("type")) {
    type = AdType(*value);
  }

  if (const auto* value = root.FindString("placement_id")) {
    placement_id = *value;
  }

  if (const auto* value = root.FindString("creative_instance_id")) {
    creative_instance_id = *value;
  }

  if (const auto* value = root.FindString("creative_set_id")) {
    creative_set_id = *value;
  }

  if (const auto* value = root.FindString("campaign_id")) {
    campaign_id = *value;
  }

  if (const auto* value = root.FindString("advertiser_id")) {
    advertiser_id = *value;
  }

  if (const auto* value = root.FindString("segment")) {
    segment = *value;
  }

  if (const auto* value = root.FindString("company_name")) {
    company_name = *value;
  }

  if (const auto* value = root.FindString("image_url")) {
    image_url = GURL(*value);
  }

  if (const auto* value = root.FindString("alt")) {
    alt = *value;
  }

  if (const auto* value = root.FindString("target_url")) {
    target_url = GURL(*value);
  }

  wallpapers.clear();
  if (const auto* wallpapers_list = root.FindList("wallpapers")) {
    for (const auto& item : *wallpapers_list) {
      if (!item.is_dict()) {
        continue;
      }

      const auto& wallpaper = item.GetDict();
      const auto* image_url = wallpaper.FindString("image_url");
      const auto* focal_point = wallpaper.FindDict("focal_point");
      if (!image_url || !focal_point) {
        continue;
      }

      const auto x = focal_point->FindInt("x");
      const auto y = focal_point->FindInt("y");
      if (!x || !y) {
        continue;
      }

      NewTabPageAdWallpaperInfo wallpaper_info;
      wallpaper_info.image_url = GURL(*image_url);
      wallpaper_info.focal_point.x = *x;
      wallpaper_info.focal_point.y = *y;

      wallpapers.push_back(wallpaper_info);
    }
  }
}

std::string NewTabPageAdInfo::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool NewTabPageAdInfo::FromJson(const std::string& json) {
  absl::optional<base::Value> document =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.has_value() || !document->is_dict()) {
    return false;
  }

  FromValue(document->GetDict());

  return true;
}

}  // namespace ads
