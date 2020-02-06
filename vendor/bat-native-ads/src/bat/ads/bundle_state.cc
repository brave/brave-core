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

  CreativeAdNotificationCategories new_ad_notification_categories;

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

        if (new_ad_notification_categories.find(category.name.GetString()) ==
            new_ad_notification_categories.end()) {
          new_ad_notification_categories.insert(
              {category.name.GetString(), {}});
        }

        new_ad_notification_categories.at(
            category.name.GetString()).push_back(info);
      }
    }
  }

  ad_notification_categories = new_ad_notification_categories;

  CreativePublisherAdCategories new_publisher_ad_categories;

  if (bundle.HasMember("publisher_ad_categories")) {
    for (const auto& category : bundle["publisher_ad_categories"].GetObject()) {
      for (const auto& creative : category.value.GetArray()) {
        CreativePublisherAdInfo info;

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

        info.size = creative["size"].GetString();
        info.creative_url = creative["creativeUrl"].GetString();
        info.target_url = helper::Uri::GetUri(
            creative["targetUrl"].GetString());

        info.creative_instance_id = creative["creativeInstanceId"].GetString();

        std::vector<std::string> channels;
        if (creative.HasMember("channels")) {
          for (const auto& site : creative["channels"].GetArray()) {
            channels.push_back(site.GetString());
          }
        }
        info.channels = channels;

        if (new_publisher_ad_categories.find(category.name.GetString()) ==
            new_publisher_ad_categories.end()) {
          new_publisher_ad_categories.insert({category.name.GetString(), {}});
        }

        new_publisher_ad_categories.at(
            category.name.GetString()).push_back(info);
      }
    }
  }

  publisher_ad_categories = new_publisher_ad_categories;

  AdConversions new_ad_conversions;

  if (bundle.HasMember("conversions")) {
    for (const auto& info : bundle["conversions"].GetArray()) {
      AdConversionTrackingInfo ad_conversion;

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

  for (const auto& category : state.ad_notification_categories) {
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

  writer->String("publisher_ad_categories");
  writer->StartObject();

  for (const auto& category : state.publisher_ad_categories) {
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

      writer->String("size");
      writer->String(ad.size.c_str());

      writer->String("creativeUrl");
      writer->String(ad.creative_url.c_str());

      writer->String("targetUrl");
      writer->String(ad.target_url.c_str());

      writer->String("creativeInstanceId");
      writer->String(ad.creative_instance_id.c_str());

      writer->String("channels");
      writer->StartArray();
      for (const auto& channel : ad.channels) {
        writer->String(channel.c_str());
      }
      writer->EndArray();

      writer->EndObject();
    }

    writer->EndArray();
  }

  writer->EndObject();

  writer->String("conversions");
  writer->StartArray();

  for (const auto& conversion : state.ad_conversions) {
    writer->StartObject();

    writer->String("creativeSetId");
    writer->String(conversion.creative_set_id.c_str());

    writer->String("type");
    writer->String(conversion.type.c_str());

    writer->String("urlPattern");
    writer->String(conversion.url_pattern.c_str());

    writer->String("observationWindow");
    writer->Uint(conversion.observation_window);

    writer->EndObject();
  }

  writer->EndArray();
  writer->EndObject();
}

}  // namespace ads
