/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_preferences.h"

#include "bat/ads/internal/filtered_ad.h"
#include "bat/ads/internal/filtered_category.h"
#include "bat/ads/internal/flagged_ad.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/saved_ad.h"

namespace ads {

AdPreferences::AdPreferences() = default;

AdPreferences::AdPreferences(
    const AdPreferences& prefs) = default;

AdPreferences::~AdPreferences() = default;

const std::string AdPreferences::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdPreferences::FromJson(
    const std::string& json,
    std::string* error_description) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    if (error_description != nullptr) {
      *error_description = helper::JSON::GetLastError(&document);
    }

    return FAILED;
  }

  for (const auto& ad : document["filtered_ads"].GetArray()) {
    if (!ad["uuid"].IsString() || !ad["creative_set_id"].IsString()) {
      return FAILED;
    }

    FilteredAd filtered_ad;
    filtered_ad.uuid = ad["uuid"].GetString();
    filtered_ad.creative_set_id = ad["creative_set_id"].GetString();
    filtered_ads.push_back(filtered_ad);
  }

  for (const auto& ad : document["filtered_categories"].GetArray()) {
    if (!ad["name"].IsString()) {
      return FAILED;
    }

    FilteredCategory filtered_category;
    filtered_category.name = ad["name"].GetString();
    filtered_categories.push_back(filtered_category);
  }

  for (const auto& ad : document["saved_ads"].GetArray()) {
    if (!ad["uuid"].IsString() || !ad["creative_set_id"].IsString()) {
      return FAILED;
    }

    SavedAd saved_ad;
    saved_ad.uuid = ad["uuid"].GetString();
    saved_ad.creative_set_id = ad["creative_set_id"].GetString();
    saved_ads.push_back(saved_ad);
  }

  for (const auto& ad : document["flagged_ads"].GetArray()) {
    if (!ad["uuid"].IsString() || !ad["creative_set_id"].IsString()) {
      return FAILED;
    }

    FlaggedAd flagged_ad;
    flagged_ad.uuid = ad["uuid"].GetString();
    flagged_ad.creative_set_id = ad["creative_set_id"].GetString();
    flagged_ads.push_back(flagged_ad);
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const AdPreferences& prefs) {
  writer->StartObject();

  writer->String("filtered_ads");
  writer->StartArray();
  for (const auto& ad : prefs.filtered_ads) {
    writer->StartObject();

    writer->String("uuid");
    writer->String(ad.uuid.c_str());

    writer->String("creative_set_id");
    writer->String(ad.creative_set_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->String("filtered_categories");
  writer->StartArray();
  for (const auto& category : prefs.filtered_categories) {
    writer->StartObject();

    writer->String("name");
    writer->String(category.name.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->String("saved_ads");
  writer->StartArray();
  for (const auto& ad : prefs.saved_ads) {
    writer->StartObject();

    writer->String("uuid");
    writer->String(ad.uuid.c_str());

    writer->String("creative_set_id");
    writer->String(ad.creative_set_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->String("flagged_ads");
  writer->StartArray();
  for (const auto& ad : prefs.flagged_ads) {
    writer->StartObject();

    writer->String("uuid");
    writer->String(ad.uuid.c_str());

    writer->String("creative_set_id");
    writer->String(ad.creative_set_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
