/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/bundle_state.h"

#include "json_helper.h"
#include "uri_helper.h"

namespace ads {

BundleState::BundleState() :
    catalog_id(""),
    catalog_version(0),
    catalog_ping(0),
    catalog_last_updated_timestamp(0),
    categories({}) {}

BundleState::BundleState(const BundleState& state):
    catalog_id(state.catalog_id),
    catalog_version(state.catalog_version),
    catalog_ping(state.catalog_ping),
    catalog_last_updated_timestamp(state.catalog_last_updated_timestamp),
    categories(state.categories) {}

BundleState::~BundleState() = default;

const std::string BundleState::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result BundleState::FromJson(
    const std::string& json,
    const std::string& json_schema,
    std::string* error_description) {
  rapidjson::Document bundle;
  bundle.Parse(json.c_str());

  auto result = helper::JSON::Validate(&bundle, json_schema);
  if (result != SUCCESS) {
    if (error_description != nullptr) {
      *error_description = helper::JSON::GetLastError(&bundle);
    }

    return result;
  }

  std::map<std::string, std::vector<AdInfo>> new_categories = {};

  if (bundle.HasMember("categories")) {
    for (const auto& category : bundle["categories"].GetObject()) {
      for (const auto& info : category.value.GetArray()) {
        AdInfo ad_info;

        if (info.HasMember("creativeSetId")) {
          ad_info.creative_set_id = info["creativeSetId"].GetString();
        }

        if (info.HasMember("campaignId")) {
          ad_info.campaign_id = info["campaignId"].GetString();
        }

        if (info.HasMember("startTimestamp")) {
          ad_info.start_timestamp = info["startTimestamp"].GetString();
        }

        if (info.HasMember("endTimestamp")) {
          ad_info.end_timestamp = info["endTimestamp"].GetString();
        }

        if (info.HasMember("dailyCap")) {
          ad_info.daily_cap = info["dailyCap"].GetUint();
        }

        if (info.HasMember("perDay")) {
          ad_info.per_day = info["perDay"].GetUint();
        }

        if (info.HasMember("totalMax")) {
          ad_info.total_max = info["totalMax"].GetUint();
        }

        std::vector<std::string> regions = {};
        if (info.HasMember("regions")) {
          for (const auto& region : info["regions"].GetArray()) {
            regions.push_back(region.GetString());
          }
        }
        ad_info.regions = regions;

        ad_info.advertiser = info["advertiser"].GetString();
        ad_info.notification_text = info["notificationText"].GetString();
        ad_info.notification_url =
          helper::Uri::GetUri(info["notificationURL"].GetString());
        ad_info.uuid = info["uuid"].GetString();

        if (new_categories.find(category.name.GetString()) ==
            new_categories.end()) {
          new_categories.insert({category.name.GetString(), {}});
        }
        new_categories.at(category.name.GetString()).push_back(ad_info);
      }
    }
  }

  categories = new_categories;

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const BundleState& state) {
  writer->StartObject();

  writer->String("categories");
  writer->StartObject();

  for (const auto& category : state.categories) {
    writer->String(category.first.c_str());
    writer->StartArray();

    for (const auto& ad : category.second) {
      writer->StartObject();

      writer->String("creativeSetId");
      writer->String(ad.creative_set_id.c_str());

      writer->String("campaignId");
      writer->String(ad.campaign_id.c_str());

      writer->String("startTimestamp");
      writer->String(ad.start_timestamp.c_str());

      writer->String("endTimestamp");
      writer->String(ad.end_timestamp.c_str());

      writer->String("dailyCap");
      writer->Uint(ad.daily_cap);

      writer->String("perDay");
      writer->Uint(ad.per_day);

      writer->String("totalMax");
      writer->Uint(ad.total_max);

      writer->String("regions");
      writer->StartArray();
      for (const auto& region : ad.regions) {
        writer->String(region.c_str());
      }
      writer->EndArray();

      writer->String("advertiser");
      writer->String(ad.advertiser.c_str());

      writer->String("notificationText");
      writer->String(ad.notification_text.c_str());

      writer->String("notificationURL");
      writer->String(ad.notification_url.c_str());

      writer->String("uuid");
      writer->String(ad.uuid.c_str());

      writer->EndObject();
    }

    writer->EndArray();
  }

  writer->EndObject();

  writer->EndObject();
}

}  // namespace ads
