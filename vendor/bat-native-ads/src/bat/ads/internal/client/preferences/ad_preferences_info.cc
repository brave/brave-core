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

  if (document.HasMember("filtered_advertisers")) {
    for (const auto& advertiser : document["filtered_advertisers"].GetArray()) {
      const auto iterator = advertiser.FindMember("id");
      if (iterator == advertiser.MemberEnd() || !iterator->value.IsString()) {
        continue;
      }

      FilteredAdvertiserInfo filtered_advertiser;
      filtered_advertiser.id = iterator->value.GetString();
      filtered_advertisers.push_back(filtered_advertiser);
    }
  }

  if (document.HasMember("filtered_categories")) {
    for (const auto& category : document["filtered_categories"].GetArray()) {
      const auto iterator = category.FindMember("name");
      if (iterator == category.MemberEnd() || !iterator->value.IsString()) {
        continue;
      }

      FilteredCategoryInfo filtered_category;
      filtered_category.name = iterator->value.GetString();
      filtered_categories.push_back(filtered_category);
    }
  }

  if (document.HasMember("saved_ads")) {
    for (const auto& ad : document["saved_ads"].GetArray()) {
      const auto iterator = ad.FindMember("creative_instance_id");
      if (iterator == ad.MemberEnd() || !iterator->value.IsString()) {
        continue;
      }

      SavedAdInfo saved_ad;
      saved_ad.creative_instance_id = iterator->value.GetString();
      saved_ads.push_back(saved_ad);
    }
  }

  if (document.HasMember("flagged_ads")) {
    for (const auto& ad : document["flagged_ads"].GetArray()) {
      const auto iterator = ad.FindMember("creative_set_id");
      if (iterator == ad.MemberEnd() || !iterator->value.IsString()) {
        continue;
      }

      FlaggedAdInfo flagged_ad;
      flagged_ad.creative_set_id = iterator->value.GetString();
      flagged_ads.push_back(flagged_ad);
    }
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const AdPreferencesInfo& info) {
  writer->StartObject();

  writer->String("filtered_advertisers");
  writer->StartArray();
  for (const auto& advertiser : info.filtered_advertisers) {
    writer->StartObject();

    writer->String("id");
    writer->String(advertiser.id.c_str());

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

    writer->String("creative_instance_id");
    writer->String(ad.creative_instance_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->String("flagged_ads");
  writer->StartArray();
  for (const auto& ad : info.flagged_ads) {
    writer->StartObject();

    writer->String("creative_set_id");
    writer->String(ad.creative_set_id.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
