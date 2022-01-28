/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_info.h"

#include "base/values.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

NewTabPageAdInfo::NewTabPageAdInfo() = default;

NewTabPageAdInfo::NewTabPageAdInfo(const NewTabPageAdInfo& info) = default;

NewTabPageAdInfo::~NewTabPageAdInfo() = default;

bool NewTabPageAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (company_name.empty() || image_url.empty() || alt.empty() ||
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

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
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
    image_url = document["image_url"].GetString();
  }

  if (document.HasMember("alt")) {
    alt = document["alt"].GetString();
  }

  // TODO(https://github.com/brave/brave-browser/issues/14015): Read wallpapers
  // JSON

  if (document.HasMember("target_url")) {
    target_url = document["target_url"].GetString();
  }

  return true;
}

base::DictionaryValue NewTabPageAdInfo::ToValue() const {
  base::DictionaryValue ad_info;

  ad_info.SetStringKey("type", std::string(type));
  ad_info.SetStringKey("uuid", uuid);
  ad_info.SetStringKey("creative_instance_id", creative_instance_id);
  ad_info.SetStringKey("creative_set_id", creative_set_id);
  ad_info.SetStringKey("campaign_id", campaign_id);
  ad_info.SetStringKey("advertiser_id", advertiser_id);
  ad_info.SetStringKey("segment", segment);
  ad_info.SetStringKey("company_name", company_name);
  ad_info.SetStringKey("image_url", image_url);
  ad_info.SetStringKey("alt", alt);

  std::vector<base::Value> wallpaper_list;
  for (const NewTabPageAdWallpaperInfo& wallpaper : wallpapers) {
    base::Value wallpaper_value(base::Value::Type::DICTIONARY);
    wallpaper_value.SetStringKey("image_url", wallpaper.image_url);
    base::Value focal_point(base::Value::Type::DICTIONARY);
    focal_point.SetIntKey("x", wallpaper.focal_point.x);
    focal_point.SetIntKey("y", wallpaper.focal_point.y);
    wallpaper_value.SetKey("focal_point", std::move(focal_point));
    wallpaper_list.push_back(std::move(wallpaper_value));
  }
  ad_info.SetKey("wallpapers", base::Value(std::move(wallpaper_list)));
  ad_info.SetStringKey("target_url", target_url);

  return ad_info;
}

void NewTabPageAdInfo::FromValue(const base::DictionaryValue& ad_info) {
  if (const std::string* value = ad_info.FindStringKey("type")) {
    type = AdType(*value);
  }

  if (const std::string* value = ad_info.FindStringKey("uuid")) {
    uuid = *value;
  }

  if (const std::string* value =
          ad_info.FindStringKey("creative_instance_id")) {
    creative_instance_id = *value;
  }

  if (const std::string* value =
          ad_info.FindStringKey("creative_instance_id")) {
    creative_instance_id = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("creative_set_id")) {
    creative_set_id = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("campaign_id")) {
    campaign_id = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("advertiser_id")) {
    advertiser_id = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("segment")) {
    segment = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("company_name")) {
    company_name = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("image_url")) {
    image_url = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("alt")) {
    alt = *value;
  }

  if (const std::string* value = ad_info.FindStringKey("target_url")) {
    target_url = *value;
  }
}

void SaveToJson(JsonWriter* writer, const NewTabPageAdInfo& info) {
  writer->StartObject();

  writer->String("type");
  const std::string& type = std::string(info.type);
  writer->String(type.c_str());

  writer->String("uuid");
  writer->String(info.uuid.c_str());

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
  writer->String(info.image_url.c_str());

  writer->String("alt");
  writer->String(info.alt.c_str());

  // TODO(https://github.com/brave/brave-browser/issues/14015): Write wallpapers
  // JSON

  writer->String("wallpapers");
  writer->StartArray();
  for (const NewTabPageAdWallpaperInfo& wallpaper_info : info.wallpapers) {
    writer->StartObject();
    writer->String("image_url");
    writer->String(wallpaper_info.image_url.c_str());
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
  writer->String(info.target_url.c_str());

  writer->EndObject();
}

}  // namespace ads
