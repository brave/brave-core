/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_state.h"

#include "base/time/time.h"
#include "bat/ads/internal/catalog/catalog_version.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"
#include "url/gurl.h"

namespace ads {

namespace {
const int64_t kDefaultCatalogPing = 2 * base::Time::kSecondsPerHour;
}  // namespace

CatalogState::CatalogState() = default;

CatalogState::CatalogState(const CatalogState& state) = default;

CatalogState::~CatalogState() = default;

Result CatalogState::FromJson(const std::string& json,
                              const std::string& json_schema) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  auto result = helper::JSON::Validate(&document, json_schema);
  if (result != SUCCESS) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return result;
  }

  std::string new_catalog_id;
  int new_version = 0;
  int64_t new_ping = kDefaultCatalogPing * base::Time::kMillisecondsPerSecond;
  CatalogCampaignList new_campaigns;
  CatalogIssuersInfo new_catalog_issuers;

  new_catalog_id = document["catalogId"].GetString();

  new_version = document["version"].GetInt();
  if (new_version != kCurrentCatalogVersion) {
    return FAILED;
  }

  new_ping = document["ping"].GetInt64();

  // Campaigns
  for (const auto& campaign : document["campaigns"].GetArray()) {
    CatalogCampaignInfo campaign_info;

    campaign_info.campaign_id = campaign["campaignId"].GetString();
    campaign_info.priority = campaign["priority"].GetUint();
    campaign_info.ptr = campaign["ptr"].GetDouble();
    campaign_info.start_at = campaign["startAt"].GetString();
    campaign_info.end_at = campaign["endAt"].GetString();
    campaign_info.daily_cap = campaign["dailyCap"].GetUint();
    campaign_info.advertiser_id = campaign["advertiserId"].GetString();

    // Geo targets
    for (const auto& geo_target : campaign["geoTargets"].GetArray()) {
      CatalogGeoTargetInfo geo_target_info;

      geo_target_info.code = geo_target["code"].GetString();
      geo_target_info.name = geo_target["name"].GetString();

      campaign_info.geo_targets.push_back(geo_target_info);
    }

    // Day parts
    for (const auto& daypart : campaign["dayParts"].GetArray()) {
      CatalogDaypartInfo daypart_info;

      daypart_info.dow = daypart["dow"].GetString();
      daypart_info.start_minute = daypart["startMinute"].GetInt();
      daypart_info.end_minute = daypart["endMinute"].GetInt();

      campaign_info.dayparts.push_back(daypart_info);
    }

    if (campaign_info.dayparts.empty()) {
      CatalogDaypartInfo daypart_info;
      campaign_info.dayparts.push_back(daypart_info);
    }

    // Creative sets
    for (const auto& creative_set : campaign["creativeSets"].GetArray()) {
      CatalogCreativeSetInfo creative_set_info;

      creative_set_info.creative_set_id =
          creative_set["creativeSetId"].GetString();

      creative_set_info.per_day = creative_set["perDay"].GetUint();

      creative_set_info.per_week = creative_set["perWeek"].GetUint();

      creative_set_info.per_month = creative_set["perMonth"].GetUint();

      creative_set_info.total_max = creative_set["totalMax"].GetUint();

      if (creative_set.HasMember("splitTestGroup")) {
        creative_set_info.split_test_group =
            creative_set["splitTestGroup"].GetString();
      }

      // Segments
      auto segments = creative_set["segments"].GetArray();
      if (segments.Size() == 0) {
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

      for (const auto& conversion_node : conversions) {
        ConversionInfo conversion;

        conversion.creative_set_id = creative_set_info.creative_set_id;
        conversion.type = conversion_node["type"].GetString();
        conversion.url_pattern = conversion_node["urlPattern"].GetString();
        conversion.observation_window =
            conversion_node["observationWindow"].GetUint();

        if (conversion_node.HasMember("conversionPublicKey")) {
          conversion.advertiser_public_key =
              conversion_node["conversionPublicKey"].GetString();
        }

        base::Time end_at_timestamp;
        if (!base::Time::FromUTCString(campaign_info.end_at.c_str(),
                                       &end_at_timestamp)) {
          continue;
        }

        base::Time expiry_timestamp =
            end_at_timestamp +
            base::TimeDelta::FromDays(conversion.observation_window);
        conversion.expiry_timestamp =
            static_cast<int64_t>(expiry_timestamp.ToDoubleT());

        creative_set_info.conversions.push_back(conversion);
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
          if (!GURL(creative_info.payload.target_url).is_valid()) {
            BLOG(1, "Invalid target URL for creative instance id "
                        << creative_instance_id);
            continue;
          }

          creative_set_info.creative_ad_notifications.push_back(creative_info);
        } else if (code == "inline_content_all_v1") {
          CatalogCreativeInlineContentAdInfo creative_info;

          creative_info.creative_instance_id = creative_instance_id;

          // Type
          creative_info.type.code = code;
          creative_info.type.name = type["name"].GetString();
          creative_info.type.platform = type["platform"].GetString();
          creative_info.type.version = type["version"].GetUint64();

          // Payload
          auto payload = creative["payload"].GetObject();
          creative_info.payload.title = payload["title"].GetString();
          creative_info.payload.description =
              payload["description"].GetString();
          creative_info.payload.image_url = payload["imageUrl"].GetString();
          if (!GURL(creative_info.payload.image_url).is_valid()) {
            BLOG(1, "Invalid image URL for creative instance id "
                        << creative_instance_id);
            continue;
          }
          creative_info.payload.dimensions = payload["dimensions"].GetString();
          creative_info.payload.cta_text = payload["ctaText"].GetString();
          creative_info.payload.target_url = payload["targetUrl"].GetString();
          if (!GURL(creative_info.payload.target_url).is_valid()) {
            BLOG(1, "Invalid target URL for creative instance id "
                        << creative_instance_id);
            continue;
          }

          creative_set_info.creative_inline_content_ads.push_back(
              creative_info);
        } else if (code == "new_tab_page_all_v1") {
          CatalogCreativeNewTabPageAdInfo creative_info;

          creative_info.creative_instance_id = creative_instance_id;

          // Type
          creative_info.type.code = code;
          creative_info.type.name = type["name"].GetString();
          creative_info.type.platform = type["platform"].GetString();
          creative_info.type.version = type["version"].GetUint64();

          // Payload
          auto payload = creative["payload"].GetObject();
          auto logo = payload["logo"].GetObject();

          creative_info.payload.company_name = logo["companyName"].GetString();
          creative_info.payload.alt = logo["alt"].GetString();
          creative_info.payload.target_url = logo["destinationUrl"].GetString();
          if (!GURL(creative_info.payload.target_url).is_valid()) {
            BLOG(1, "Invalid target URL for creative instance id "
                        << creative_instance_id);
            continue;
          }

          creative_set_info.creative_new_tab_page_ads.push_back(creative_info);
        } else if (code == "promoted_content_all_v1") {
          CatalogCreativePromotedContentAdInfo creative_info;

          creative_info.creative_instance_id = creative_instance_id;

          // Type
          creative_info.type.code = code;
          creative_info.type.name = type["name"].GetString();
          creative_info.type.platform = type["platform"].GetString();
          creative_info.type.version = type["version"].GetUint64();

          // Payload
          auto payload = creative["payload"].GetObject();
          creative_info.payload.title = payload["title"].GetString();
          creative_info.payload.description =
              payload["description"].GetString();
          creative_info.payload.target_url = payload["feed"].GetString();
          if (!GURL(creative_info.payload.target_url).is_valid()) {
            BLOG(1, "Invalid target URL for creative instance id "
                        << creative_instance_id);
            continue;
          }

          creative_set_info.creative_promoted_content_ads.push_back(
              creative_info);
        } else if (code == "in_page_all_v1") {
          // TODO(tmancey): https://github.com/brave/brave-browser/issues/7298
          continue;
        } else {
          // Unknown type
          NOTREACHED();
          continue;
        }
      }

      campaign_info.creative_sets.push_back(creative_set_info);
    }

    new_campaigns.push_back(campaign_info);
  }

  // Issuers
  for (const auto& issuer : document["issuers"].GetArray()) {
    CatalogIssuerInfo catalog_issuer_info;

    std::string name = issuer["name"].GetString();
    std::string public_key = issuer["publicKey"].GetString();

    if (name == "confirmation") {
      new_catalog_issuers.public_key = public_key;
      continue;
    }

    catalog_issuer_info.name = name;
    catalog_issuer_info.public_key = public_key;

    new_catalog_issuers.issuers.push_back(catalog_issuer_info);
  }

  catalog_id = new_catalog_id;
  version = new_version;
  ping = new_ping;
  campaigns = new_campaigns;
  catalog_issuers = new_catalog_issuers;

  return SUCCESS;
}

}  // namespace ads
