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

  if (bundle.HasMember("creative_ad_notifications")) {
    for (const auto& creative_ad_notification :
        bundle["creative_ad_notifications"].GetObject()) {
      for (const auto& creative : creative_ad_notification.value.GetArray()) {
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

        const std::string category = creative_ad_notification.name.GetString();
        if (new_creative_ad_notifications.find(category) ==
            new_creative_ad_notifications.end()) {
          new_creative_ad_notifications.insert({category, {}});
        }

        new_creative_ad_notifications.at(category).push_back(info);
      }
    }
  }

  creative_ad_notifications = new_creative_ad_notifications;

  CreativePublisherAdMap new_creative_publisher_ads;

  if (bundle.HasMember("creative_publisher_ads")) {
    for (const auto& creative_publisher_ad :
        bundle["creative_publisher_ads"].GetObject()) {
      for (const auto& creative : creative_publisher_ad.value.GetArray()) {
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

        const std::string category = creative_publisher_ad.name.GetString();
        if (new_creative_publisher_ads.find(category) ==
            new_creative_publisher_ads.end()) {
          new_creative_publisher_ads.insert({category, {}});
        }

        new_creative_publisher_ads.at(category).push_back(info);
      }
    }
  }

  creative_publisher_ads = new_creative_publisher_ads;

  AdConversionList new_ad_conversions;

  if (bundle.HasMember("ad_conversions")) {
    for (const auto& ad_conversion : bundle["ad_conversions"].GetArray()) {
      AdConversionInfo info;

      if (ad_conversion.HasMember("creativeSetId")) {
        info.creative_set_id =
            ad_conversion["creativeSetId"].GetString();
      }

      if (ad_conversion.HasMember("type")) {
        info.type =
            ad_conversion["type"].GetString();
      }

      if (ad_conversion.HasMember("urlPattern")) {
        info.url_pattern =
            ad_conversion["urlPattern"].GetString();
      }

      if (ad_conversion.HasMember("observationWindow")) {
        info.observation_window =
            ad_conversion["observationWindow"].GetUint();
      }

      new_ad_conversions.push_back(info);
    }
  }

  ad_conversions = new_ad_conversions;

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const BundleState& state) {
  writer->StartObject();

  writer->String("creative_ad_notifications");
  writer->StartObject();

  for (const auto& creative_ad_notification : state.creative_ad_notifications) {
    const std::string category = creative_ad_notification.first.c_str();
    writer->String(category.c_str());
    writer->StartArray();

    for (const auto& ad : creative_ad_notification.second) {
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

  writer->String("creative_publisher_ads");
  writer->StartObject();

  for (const auto& creative_publisher_ad : state.creative_publisher_ads) {
    const std::string category = creative_publisher_ad.first.c_str();
    writer->String(category.c_str());
    writer->StartArray();

    for (const auto& ad : creative_publisher_ad.second) {
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

  writer->String("ad_conversions");
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
