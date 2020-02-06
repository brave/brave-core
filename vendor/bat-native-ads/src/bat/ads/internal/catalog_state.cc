/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog_state.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/static_values.h"

namespace ads {

CatalogState::CatalogState() = default;

CatalogState::CatalogState(
    const CatalogState& state) = default;

CatalogState::~CatalogState() = default;

Result CatalogState::FromJson(
    const std::string& json,
    const std::string& json_schema,
    std::string* error_description) {
  rapidjson::Document catalog;
  catalog.Parse(json.c_str());

  auto result = helper::JSON::Validate(&catalog, json_schema);
  if (result != SUCCESS) {
    if (error_description != nullptr) {
      *error_description = helper::JSON::GetLastError(&catalog);
    }

    return result;
  }

  std::string new_catalog_id = "";
  uint64_t new_version = 0;
  uint64_t new_ping = kDefaultCatalogPing * base::Time::kMillisecondsPerSecond;
  std::vector<CatalogCampaignInfo> new_campaigns;
  IssuersInfo new_issuers;

  new_catalog_id = catalog["catalogId"].GetString();

  new_version = catalog["version"].GetUint64();
  if (new_version != kCatalogVersion) {
    if (error_description != nullptr) {
      *error_description = "Unsupported catalog verion";
    }

    return FAILED;
  }

  new_ping = catalog["ping"].GetUint64();

  // Campaigns
  for (const auto& campaign : catalog["campaigns"].GetArray()) {
    CatalogCampaignInfo campaign_info;

    campaign_info.campaign_id = campaign["campaignId"].GetString();
    campaign_info.advertiser_id = campaign["advertiserId"].GetString();
    campaign_info.priority = campaign["priority"].GetUint();
    campaign_info.start_at = campaign["startAt"].GetString();
    campaign_info.end_at = campaign["endAt"].GetString();
    campaign_info.daily_cap = campaign["dailyCap"].GetUint();

    // Geo targets
    for (const auto& geo_target : campaign["geoTargets"].GetArray()) {
      CatalogGeoTargetInfo geo_target_info;

      geo_target_info.code = geo_target["code"].GetString();
      geo_target_info.name = geo_target["name"].GetString();

      campaign_info.geo_targets.push_back(geo_target_info);
    }

    // Day parts
    for (const auto& day_part : campaign["dayParts"].GetArray()) {
      CatalogDayPartInfo day_part_info;

      day_part_info.dow = day_part["dow"].GetString();
      day_part_info.start_minute = day_part["startMinute"].GetUint();
      day_part_info.end_minute = day_part["endMinute"].GetUint();

      campaign_info.day_parts.push_back(day_part_info);
    }

    // Creative sets
    for (const auto& creative_set : campaign["creativeSets"].GetArray()) {
      CatalogCreativeSetInfo creative_set_info;

      creative_set_info.creative_set_id =
          creative_set["creativeSetId"].GetString();

      creative_set_info.per_day = creative_set["perDay"].GetUint();

      creative_set_info.total_max = creative_set["totalMax"].GetUint();

      // Segments
      auto segments = creative_set["segments"].GetArray();
      if (segments.Size() == 0) {
        if (error_description != nullptr) {
          *error_description = "Catalog invalid: No segments for creativeSet "
              "with creativeSetId: " + creative_set_info.creative_set_id;
        }

        continue;
      }

      for (const auto& segment : segments) {
        CatalogSegmentInfo segment_info;

        segment_info.code = segment["code"].GetString();
        segment_info.name = segment["name"].GetString();

        creative_set_info.segments.push_back(segment_info);
      }

      // Oses
      auto oses = creative_set["oses"].GetArray();

      for (const auto& os : oses) {
        CatalogOsInfo os_info;

        os_info.code = os["code"].GetString();
        os_info.name = os["name"].GetString();

        creative_set_info.oses.push_back(os_info);
      }

      // Conversions
      const auto conversions = creative_set["conversions"].GetArray();

      for (const auto& conversion : conversions) {
        AdConversionTrackingInfo ad_conversion;

        ad_conversion.creative_set_id = creative_set_info.creative_set_id;
        ad_conversion.type = conversion["type"].GetString();
        ad_conversion.url_pattern = conversion["urlPattern"].GetString();
        ad_conversion.observation_window =
            conversion["observationWindow"].GetUint();

        creative_set_info.ad_conversions.push_back(ad_conversion);
      }

      // Creatives
      for (const auto& creative : creative_set["creatives"].GetArray()) {
        std::string creative_instance_id =
            creative["creativeInstanceId"].GetString();

        // Type
        auto type = creative["type"].GetObject();

        std::string code = type["code"].GetString();
        if (code == "notification_all_v1") {
          CatalogCreativeAdNotificationInfo creative_info;

          creative_info.creative_instance_id = creative_instance_id;

          // Type
          creative_info.type.code = code;
          creative_info.type.name = type["name"].GetString();
          creative_info.type.platform = type["platform"].GetString();
          creative_info.type.version = type["version"].GetUint64();

          // Payload
          auto payload = creative["payload"].GetObject();
          creative_info.payload.body = payload["body"].GetString();
          creative_info.payload.title = payload["title"].GetString();
          creative_info.payload.target_url = payload["targetUrl"].GetString();

          creative_set_info.ad_notification_creatives.push_back(creative_info);
        } else if (code == "in_page_all_v1") {
          CatalogCreativePublisherAdInfo creative_info;

          creative_info.creative_instance_id = creative_instance_id;

          // Type
          creative_info.type.code = code;
          creative_info.type.name = type["name"].GetString();
          creative_info.type.platform = type["platform"].GetString();
          creative_info.type.version = type["version"].GetUint64();

          // Payload
          auto payload = creative["payload"].GetObject();
          creative_info.payload.size = payload["size"].GetString();
          creative_info.payload.creative_url =
              payload["creativeUrl"].GetString();
          creative_info.payload.target_url = payload["targetUrl"].GetString();

          // Channels
          for (const auto& channel : creative_set["channels"].GetArray()) {
            CatalogPublisherAdChannelInfo channel_info;
            channel_info.name = channel.GetString();
            creative_info.channels.push_back(channel_info);
          }

          creative_set_info.publisher_ad_creatives.push_back(creative_info);
        } else {
          if (error_description != nullptr) {
            *error_description = "Catalog invalid: Invalid " + code
                +" creative for creativeInstanceId: " + creative_instance_id;
          }

          continue;
        }
      }

      campaign_info.creative_sets.push_back(creative_set_info);
    }

    new_campaigns.push_back(campaign_info);
  }

  // Issuers
  for (const auto& issuer : catalog["issuers"].GetArray()) {
    IssuerInfo issuer_info;

    std::string name = issuer["name"].GetString();
    std::string public_key = issuer["publicKey"].GetString();

    if (name == "confirmation") {
      new_issuers.public_key = public_key;
      continue;
    }

    issuer_info.name = name;
    issuer_info.public_key = public_key;

    new_issuers.issuers.push_back(issuer_info);
  }

  catalog_id = new_catalog_id;
  version = new_version;
  ping = new_ping;
  campaigns = new_campaigns;
  issuers = new_issuers;

  return SUCCESS;
}

}  // namespace ads
