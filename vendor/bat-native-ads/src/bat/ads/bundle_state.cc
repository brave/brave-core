/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/bundle_state.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/uri_helper.h"

namespace ads {

BundleState::BundleState() = default;

BundleState::BundleState(
    const BundleState& state) = default;

BundleState::~BundleState() = default;

std::string BundleState::ToJson() const {
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

  CreativeAdNotificationMap new_creative_ad_notifications;

  if (bundle.HasMember("ad_notification_categories")) {
    for (const auto& category :
        bundle["ad_notification_categories"].GetObject()) {
      for (const auto& creative : category.value.GetArray()) {
        CreativeAdNotificationInfo info;

        if (creative.HasMember("creativeSetId")) {
          info.creative_set_id = creative["creativeSetId"].GetString();
        }

        if (creative.HasMember("campaignId")) {
          info.campaign_id = creative["campaignId"].GetString();
        }

        if (creative.HasMember("startAtTimestamp")) {
          info.start_at_timestamp = creative["startAtTimestamp"].GetString();
        }

        if (creative.HasMember("endAtTimestamp")) {
          info.end_at_timestamp = creative["endAtTimestamp"].GetString();
        }

        if (creative.HasMember("dailyCap")) {
          info.daily_cap = creative["dailyCap"].GetUint();
        }

        if (creative.HasMember("advertiserId")) {
          info.advertiser_id = creative["advertiserId"].GetString();
        }

        if (creative.HasMember("perDay")) {
          info.per_day = creative["perDay"].GetUint();
        }

        if (creative.HasMember("totalMax")) {
          info.total_max = creative["totalMax"].GetUint();
        }

        if (creative.HasMember("category")) {
          info.category = creative["category"].GetString();
        }

        std::vector<std::string> geo_targets;
        if (creative.HasMember("geoTargets")) {
          for (const auto& geo_target : creative["geoTargets"].GetArray()) {
            geo_targets.push_back(geo_target.GetString());
          }
        }
        info.geo_targets = geo_targets;

        info.title = creative["title"].GetString();
        info.body = creative["body"].GetString();
        info.target_url = helper::Uri::GetUri(
             creative["targetUrl"].GetString());
        info.creative_instance_id = creative["creativeInstanceId"].GetString();

        if (new_creative_ad_notifications.find(category.name.GetString()) ==
                new_creative_ad_notifications.end()) {
          new_creative_ad_notifications.insert({category.name.GetString(), {}});
        }

        new_creative_ad_notifications.at(
            category.name.GetString()).push_back(info);
      }
    }
  }

  creative_ad_notifications = new_creative_ad_notifications;

  AdConversionList new_ad_conversions;

  if (bundle.HasMember("conversions")) {
    for (const auto& info : bundle["conversions"].GetArray()) {
      AdConversionInfo ad_conversion;

      if (info.HasMember("creativeSetId")) {
        ad_conversion.creative_set_id =
            info["creativeSetId"].GetString();
      }

      if (info.HasMember("type")) {
        ad_conversion.type =
            info["type"].GetString();
      }

      if (info.HasMember("urlPattern")) {
        ad_conversion.url_pattern =
            info["urlPattern"].GetString();
      }

      if (info.HasMember("observationWindow")) {
        ad_conversion.observation_window =
            info["observationWindow"].GetUint();
      }

      new_ad_conversions.push_back(ad_conversion);
    }
  }

  ad_conversions = new_ad_conversions;

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const BundleState& state) {
  writer->StartObject();

  writer->String("ad_notification_categories");
  writer->StartObject();

  for (const auto& category : state.creative_ad_notifications) {
    writer->String(category.first.c_str());
    writer->StartArray();

    for (const auto& ad : category.second) {
      writer->StartObject();

      writer->String("creativeSetId");
      writer->String(ad.creative_set_id.c_str());

      writer->String("campaignId");
      writer->String(ad.campaign_id.c_str());

      writer->String("startAtTimestamp");
      writer->String(ad.start_at_timestamp.c_str());

      writer->String("endAtTimestamp");
      writer->String(ad.end_at_timestamp.c_str());

      writer->String("dailyCap");
      writer->Uint(ad.daily_cap);

      writer->String("advertiserId");
      writer->String(ad.advertiser_id.c_str());

      writer->String("perDay");
      writer->Uint(ad.per_day);

      writer->String("totalMax");
      writer->Uint(ad.total_max);

      writer->String("category");
      writer->String(ad.category.c_str());

      writer->String("geoTargets");
      writer->StartArray();
      for (const auto& geo_target : ad.geo_targets) {
        writer->String(geo_target.c_str());
      }
      writer->EndArray();

      writer->String("title");
      writer->String(ad.title.c_str());

      writer->String("body");
      writer->String(ad.body.c_str());

      writer->String("targetUrl");
      writer->String(ad.target_url.c_str());

      writer->String("creativeInstanceId");
      writer->String(ad.creative_instance_id.c_str());

      writer->EndObject();
    }

    writer->EndArray();
  }

  writer->EndObject();

  writer->String("conversions");
  writer->StartArray();

  for (const auto& ad_conversion : state.ad_conversions) {
    writer->StartObject();

    writer->String("creativeSetId");
    writer->String(ad_conversion.creative_set_id.c_str());

    writer->String("type");
    writer->String(ad_conversion.type.c_str());

    writer->String("urlPattern");
    writer->String(ad_conversion.url_pattern.c_str());

    writer->String("observationWindow");
    writer->Uint(ad_conversion.observation_window);

    writer->EndObject();
  }

  writer->EndArray();
  writer->EndObject();
}

}  // namespace ads
