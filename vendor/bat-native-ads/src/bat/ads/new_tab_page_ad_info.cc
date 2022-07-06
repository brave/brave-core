/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_info.h"

#include <tuple>

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/json/json_helper.h"

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

std::string NewTabPageAdInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool NewTabPageAdInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("type")) {
    type = AdType(document["type"].GetString());
  }

  if (document.HasMember("placement_id")) {
    placement_id = document["placement_id"].GetString();
  }

  if (document.HasMember("creative_instance_id")) {
    creative_instance_id = document["creative_instance_id"].GetString();
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("campaign_id")) {
    campaign_id = document["campaign_id"].GetString();
  }

  if (document.HasMember("advertiser_id")) {
    advertiser_id = document["advertiser_id"].GetString();
  }

  if (document.HasMember("segment")) {
    segment = document["segment"].GetString();
  }

  if (document.HasMember("company_name")) {
    company_name = document["company_name"].GetString();
  }

  if (document.HasMember("image_url")) {
    image_url = GURL(document["image_url"].GetString());
  }

  if (document.HasMember("alt")) {
    alt = document["alt"].GetString();
  }

  wallpapers.clear();
  if (document.HasMember("wallpapers")) {
    for (const auto& wallpaper : document["wallpapers"].GetArray()) {
      NewTabPageAdWallpaperInfo wallpaper_info;
      auto iterator = wallpaper.FindMember("image_url");
      if (iterator == wallpaper.MemberEnd() || !iterator->value.IsString()) {
        continue;
      }
      wallpaper_info.image_url = GURL(iterator->value.GetString());

      iterator = wallpaper.FindMember("focal_point");
      if (iterator == wallpaper.MemberEnd() || !iterator->value.IsObject()) {
        continue;
      }

      auto focal_point = iterator->value.GetObject();
      iterator = focal_point.FindMember("x");
      if (iterator == focal_point.MemberEnd() || !iterator->value.IsInt()) {
        continue;
      }
      wallpaper_info.focal_point.x = iterator->value.GetInt();

      iterator = focal_point.FindMember("y");
      if (iterator == focal_point.MemberEnd() || !iterator->value.IsInt()) {
        continue;
      }
      wallpaper_info.focal_point.y = iterator->value.GetInt();

      wallpapers.push_back(wallpaper_info);
    }
  }

  if (document.HasMember("target_url")) {
    target_url = GURL(document["target_url"].GetString());
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const NewTabPageAdInfo& info) {
  writer->StartObject();

  writer->String("type");
  writer->String(info.type.ToString().c_str());

  writer->String("placement_id");
  writer->String(info.placement_id.c_str());

  writer->String("creative_instance_id");
  writer->String(info.creative_instance_id.c_str());

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("campaign_id");
  writer->String(info.campaign_id.c_str());

  writer->String("advertiser_id");
  writer->String(info.advertiser_id.c_str());

  writer->String("segment");
  writer->String(info.segment.c_str());

  writer->String("company_name");
  writer->String(info.company_name.c_str());

  writer->String("image_url");
  writer->String(info.image_url.spec().c_str());

  writer->String("alt");
  writer->String(info.alt.c_str());

  writer->String("wallpapers");
  writer->StartArray();
  for (const NewTabPageAdWallpaperInfo& wallpaper_info : info.wallpapers) {
    writer->StartObject();
    writer->String("image_url");
    writer->String(wallpaper_info.image_url.spec().c_str());
    {
      writer->String("focal_point");
      writer->StartObject();
      writer->String("x");
      writer->Int(wallpaper_info.focal_point.x);
      writer->String("y");
      writer->Int(wallpaper_info.focal_point.y);
      writer->EndObject();
    }
    writer->EndObject();
  }
  writer->EndArray();

  writer->String("target_url");
  writer->String(info.target_url.spec().c_str());

  writer->EndObject();
}

}  // namespace ads
