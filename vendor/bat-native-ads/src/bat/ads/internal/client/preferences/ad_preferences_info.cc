/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/preferences/ad_preferences_info.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

AdPreferencesInfo::AdPreferencesInfo() = default;

AdPreferencesInfo::AdPreferencesInfo(const AdPreferencesInfo& info) = default;

AdPreferencesInfo::~AdPreferencesInfo() = default;

std::string AdPreferencesInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool AdPreferencesInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  for (const auto& ad : document["filtered_ads"].GetArray()) {
    if (!ad["uuid"].IsString() || !ad["creative_set_id"].IsString()) {
      return false;
    }

    FilteredAdInfo filtered_ad;
    filtered_ad.creative_instance_id = ad["uuid"].GetString();
    filtered_ad.creative_set_id = ad["creative_set_id"].GetString();
    filtered_ads.push_back(filtered_ad);
  }

  for (const auto& ad : document["filtered_categories"].GetArray()) {
    if (!ad["name"].IsString()) {
      return false;
    }

    FilteredCategoryInfo filtered_category;
    filtered_category.name = ad["name"].GetString();
    filtered_categories.push_back(filtered_category);
  }

  for (const auto& ad : document["saved_ads"].GetArray()) {
    if (!ad["uuid"].IsString() || !ad["creative_set_id"].IsString()) {
      return false;
    }

    SavedAdInfo saved_ad;
    saved_ad.creative_instance_id = ad["uuid"].GetString();
    saved_ad.creative_set_id = ad["creative_set_id"].GetString();
    saved_ads.push_back(saved_ad);
  }

  for (const auto& ad : document["flagged_ads"].GetArray()) {
    if (!ad["uuid"].IsString() || !ad["creative_set_id"].IsString()) {
      return false;
    }

    FlaggedAdInfo flagged_ad;
    flagged_ad.creative_instance_id = ad["uuid"].GetString();
    flagged_ad.creative_set_id = ad["creative_set_id"].GetString();
    flagged_ads.push_back(flagged_ad);
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const AdPreferencesInfo& info) {
  writer->StartObject();

  writer->String("filtered_ads");
  writer->StartArray();
  for (const auto& ad : info.filtered_ads) {
    writer->StartObject();

    writer->String("uuid");
    writer->String(ad.creative_instance_id.c_str());

    writer->String("creative_set_id");
    writer->String(ad.creative_set_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->String("filtered_categories");
  writer->StartArray();
  for (const auto& category : info.filtered_categories) {
    writer->StartObject();

    writer->String("name");
    writer->String(category.name.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->String("saved_ads");
  writer->StartArray();
  for (const auto& ad : info.saved_ads) {
    writer->StartObject();

    writer->String("uuid");
    writer->String(ad.creative_instance_id.c_str());

    writer->String("creative_set_id");
    writer->String(ad.creative_set_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->String("flagged_ads");
  writer->StartArray();
  for (const auto& ad : info.flagged_ads) {
    writer->StartObject();

    writer->String("uuid");
    writer->String(ad.creative_instance_id.c_str());

    writer->String("creative_set_id");
    writer->String(ad.creative_set_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
