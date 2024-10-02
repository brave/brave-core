/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_json_reader.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_campaign_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_conversion_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/json/json_helper.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "url/gurl.h"

namespace brave_ads::json::reader {

// TODO(https://github.com/brave/brave-browser/issues/25987): Reduce cognitive
// complexity.
std::optional<CatalogInfo> ReadCatalog(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  const std::string json_schema =
      GetAdsClient()->LoadDataResource(kCatalogJsonSchemaDataResourceName);

  if (!helper::json::Validate(&document, json_schema)) {
    BLOG(1, helper::json::GetLastError(&document));
    return std::nullopt;
  }

  CatalogInfo catalog;

  catalog.id = document["catalogId"].GetString();
  if (catalog.id.empty()) {
    BLOG(1, "Invalid catalog id");
    return std::nullopt;
  }

  catalog.version = document["version"].GetInt();

  const int ping = document["ping"].GetInt();
  catalog.ping = base::Milliseconds(ping);

  // Campaigns
  const auto& campaigns_node = document["campaigns"].GetArray();
  catalog.campaigns.reserve(campaigns_node.Size());

  for (const auto& campaign_node : campaigns_node) {
    CatalogCampaignInfo campaign;

    campaign.id = campaign_node["campaignId"].GetString();
    if (campaign.id.empty()) {
      BLOG(1, "Invalid campaign id");
      continue;
    }
    campaign.priority = campaign_node["priority"].GetInt();

    campaign.pass_through_rate = campaign_node["ptr"].GetDouble();

    campaign.start_at = campaign_node["startAt"].GetString();
    campaign.end_at = campaign_node["endAt"].GetString();

    campaign.daily_cap = campaign_node["dailyCap"].GetInt();

    campaign.advertiser_id = campaign_node["advertiserId"].GetString();
    if (campaign.advertiser_id.empty()) {
      BLOG(1, "Invalid advertiser id");
      continue;
    }

    // Geo targets
    const auto& geo_targets_node = campaign_node["geoTargets"].GetArray();
    campaign.geo_targets.reserve(geo_targets_node.Size());

    for (const auto& geo_target_node : geo_targets_node) {
      campaign.geo_targets.push_back(CatalogGeoTargetInfo{
          .code = geo_target_node["code"].GetString(),
          .name = geo_target_node["name"].GetString(),
      });
    }

    // Dayparts
    const auto& dayparts_node = campaign_node["dayParts"].GetArray();
    campaign.dayparts.reserve(dayparts_node.Size());

    for (const auto& daypart_node : dayparts_node) {
      campaign.dayparts.push_back(CatalogDaypartInfo{
          .days_of_week = daypart_node["dow"].GetString(),
          .start_minute = daypart_node["startMinute"].GetInt(),
          .end_minute = daypart_node["endMinute"].GetInt()});
    }

    if (campaign.dayparts.empty()) {
      campaign.dayparts.push_back(CatalogDaypartInfo{});
    }

    // Creative sets
    const auto& creative_sets_node = campaign_node["creativeSets"].GetArray();
    campaign.creative_sets.reserve(creative_sets_node.Size());

    for (const auto& creative_set_node : creative_sets_node) {
      CatalogCreativeSetInfo creative_set;

      creative_set.id = creative_set_node["creativeSetId"].GetString();
      if (creative_set.id.empty()) {
        BLOG(1, "Invalid creative set id");
        continue;
      }

      creative_set.per_day = creative_set_node["perDay"].GetInt();
      creative_set.per_week = creative_set_node["perWeek"].GetInt();
      creative_set.per_month = creative_set_node["perMonth"].GetInt();

      creative_set.total_max = creative_set_node["totalMax"].GetInt();

      const char* const value = creative_set_node["value"].GetString();
      if (!base::StringToDouble(value, &creative_set.value)) {
        BLOG(1, "Failed to parse creative set value " << value);
        continue;
      }

      if (creative_set_node.HasMember("splitTestGroup")) {
        creative_set.split_test_group =
            creative_set_node["splitTestGroup"].GetString();
      }

      // Segments
      const auto& segments_node = creative_set_node["segments"].GetArray();
      if (segments_node.Size() == 0) {
        continue;
      }
      creative_set.segments.reserve(segments_node.Size());

      for (const auto& segment_node : segments_node) {
        const std::string& code = segment_node["code"].GetString();
        if (code.empty()) {
          BLOG(1, "Failed to parse empty segment code value");
          continue;
        }

        const std::string& name = segment_node["name"].GetString();
        if (name.empty()) {
          BLOG(1, "Failed to parse empty segment name value");
          continue;
        }

        creative_set.segments.push_back(
            CatalogSegmentInfo{.code = code, .name = name});
      }

      // Oses
      const auto& oses_node = creative_set_node["oses"].GetArray();
      creative_set.oses.reserve(oses_node.Size());

      for (const auto& os_node : oses_node) {
        creative_set.oses.push_back(
            CatalogOsInfo{.code = os_node["code"].GetString(),
                          .name = os_node["name"].GetString()});
      }

      // Conversions
      const auto& conversions_node =
          creative_set_node["conversions"].GetArray();
      creative_set.conversions.reserve(conversions_node.Size());

      for (const auto& conversion_node : conversions_node) {
        CatalogConversionInfo conversion;

        conversion.creative_set_id = creative_set.id;

        conversion.url_pattern = conversion_node["urlPattern"].GetString();
        if (!ShouldSupportUrl(GURL(conversion.url_pattern))) {
          BLOG(1, "Creative set conversion URL pattern for creative set id "
                      << conversion.creative_set_id << " is unsupported");
          continue;
        }

        if (conversion_node.HasMember("conversionPublicKey")) {
          conversion.verifiable_advertiser_public_key_base64 =
              conversion_node["conversionPublicKey"].GetString();
        }

        conversion.observation_window =
            base::Days(conversion_node["observationWindow"].GetInt());

        base::Time end_at;
        if (!base::Time::FromUTCString(campaign.end_at.c_str(), &end_at)) {
          BLOG(1, "Failed to parse campaign end_at value " << campaign.end_at);
          continue;
        }

        conversion.expire_at = end_at + conversion.observation_window;

        creative_set.conversions.push_back(conversion);
      }

      // Creatives
      for (const auto& creative_node :
           creative_set_node["creatives"].GetArray()) {
        const std::string creative_instance_id =
            creative_node["creativeInstanceId"].GetString();
        if (creative_instance_id.empty()) {
          BLOG(1, "Invalid creative instance id");
          continue;
        }

        // Type
        const auto& type = creative_node["type"].GetObject();

        const std::string code = type["code"].GetString();
        if (code == "notification_all_v1") {
          CatalogCreativeNotificationAdInfo creative;

          creative.instance_id = creative_instance_id;

          // Type
          creative.type.code = code;
          creative.type.name = type["name"].GetString();
          creative.type.platform = type["platform"].GetString();
          creative.type.version = type["version"].GetInt();

          // Payload
          const auto& payload = creative_node["payload"].GetObject();
          creative.payload = CatalogNotificationAdPayloadInfo{
              .body = payload["body"].GetString(),
              .title = payload["title"].GetString(),
              .target_url = GURL(payload["targetUrl"].GetString())};
          if (!ShouldSupportUrl(creative.payload.target_url)) {
            BLOG(1, "Target URL for creative instance id "
                        << creative_instance_id << " is unsupported");
            continue;
          }

          creative_set.conversions.erase(
              base::ranges::remove_if(
                  creative_set.conversions,
                  [&creative_set,
                   &creative](const CatalogConversionInfo& conversion) {
                    const GURL conversion_url_pattern =
                        GURL(conversion.url_pattern);
                    return conversion.creative_set_id == creative_set.id &&
                           (!ShouldSupportUrl(conversion_url_pattern) ||
                            !SameDomainOrHost(creative.payload.target_url,
                                              conversion_url_pattern));
                  }),
              creative_set.conversions.cend());

          creative_set.creative_notification_ads.push_back(creative);
        } else if (code == "inline_content_all_v1") {
          CatalogCreativeInlineContentAdInfo creative;

          creative.instance_id = creative_instance_id;

          // Type
          creative.type.code = code;
          creative.type.name = type["name"].GetString();
          creative.type.platform = type["platform"].GetString();
          creative.type.version = type["version"].GetInt();

          // Payload
          const auto& payload = creative_node["payload"].GetObject();
          creative.payload.title = payload["title"].GetString();
          creative.payload.description = payload["description"].GetString();
          creative.payload.image_url = GURL(payload["imageUrl"].GetString());
          if (!ShouldSupportUrl(creative.payload.image_url)) {
            BLOG(1, "Image URL for creative instance id "
                        << creative_instance_id << " is unsupported");
            continue;
          }
          creative.payload.dimensions = payload["dimensions"].GetString();
          creative.payload.cta_text = payload["ctaText"].GetString();
          creative.payload.target_url = GURL(payload["targetUrl"].GetString());
          if (!ShouldSupportUrl(creative.payload.target_url)) {
            BLOG(1, "Target URL for creative instance id "
                        << creative_instance_id << " is unsupported");
            continue;
          }

          creative_set.conversions.erase(
              base::ranges::remove_if(
                  creative_set.conversions,
                  [&creative_set,
                   &creative](const CatalogConversionInfo& conversion) {
                    const GURL conversion_url_pattern =
                        GURL(conversion.url_pattern);
                    return conversion.creative_set_id == creative_set.id &&
                           (!ShouldSupportUrl(conversion_url_pattern) ||
                            !SameDomainOrHost(creative.payload.target_url,
                                              conversion_url_pattern));
                  }),
              creative_set.conversions.cend());

          creative_set.creative_inline_content_ads.push_back(creative);
        } else if (code == "new_tab_page_all_v1") {
          CatalogCreativeNewTabPageAdInfo creative;

          creative.instance_id = creative_instance_id;

          // Type
          creative.type.code = code;
          creative.type.name = type["name"].GetString();
          creative.type.platform = type["platform"].GetString();
          creative.type.version = type["version"].GetInt();

          // Payload
          const auto& payload = creative_node["payload"].GetObject();
          const auto& logo = payload["logo"].GetObject();
          creative.payload.company_name = logo["companyName"].GetString();
          creative.payload.image_url = GURL(logo["imageUrl"].GetString());
          if (!ShouldSupportUrl(creative.payload.image_url)) {
            BLOG(1, "Image URL for creative instance id "
                        << creative_instance_id << " is unsupported");
            continue;
          }
          creative.payload.alt = logo["alt"].GetString();
          creative.payload.target_url =
              GURL(logo["destinationUrl"].GetString());
          if (!ShouldSupportUrl(creative.payload.target_url)) {
            BLOG(1, "Target URL for creative instance id "
                        << creative_instance_id << " is unsupported");
            continue;
          }

          creative_set.conversions.erase(
              base::ranges::remove_if(
                  creative_set.conversions,
                  [&creative_set,
                   &creative](const CatalogConversionInfo& conversion) {
                    const GURL conversion_url_pattern =
                        GURL(conversion.url_pattern);
                    return conversion.creative_set_id == creative_set.id &&
                           (!ShouldSupportUrl(conversion_url_pattern) ||
                            !SameDomainOrHost(creative.payload.target_url,
                                              conversion_url_pattern));
                  }),
              creative_set.conversions.cend());

          for (const auto& wallpaper_node : payload["wallpapers"].GetArray()) {
            CatalogNewTabPageAdWallpaperInfo wallpaper;
            wallpaper.image_url = GURL(wallpaper_node["imageUrl"].GetString());
            if (!ShouldSupportUrl(wallpaper.image_url)) {
              BLOG(1, "Image URL for creative instance id "
                          << creative_instance_id << " is unsupported");
              continue;
            }
            wallpaper.focal_point = CatalogNewTabPageAdWallpaperFocalPointInfo{
                .x = wallpaper_node["focalPoint"]["x"].GetInt(),
                .y = wallpaper_node["focalPoint"]["y"].GetInt()};

            if (wallpaper_node.HasMember("conditionMatchers")) {
              if (wallpaper_node["conditionMatchers"].IsArray()) {
                for (const auto& condition_matchers_node :
                     wallpaper_node["conditionMatchers"].GetArray()) {
                  if (condition_matchers_node["prefPath"].IsString() &&
                      condition_matchers_node["condition"].IsString()) {
                    wallpaper.condition_matchers.emplace(
                        condition_matchers_node["prefPath"].GetString(),
                        condition_matchers_node["condition"].GetString());
                  }
                }
              }
            }
            creative.payload.wallpapers.push_back(wallpaper);
          }

          if (creative.payload.wallpapers.empty()) {
            BLOG(1, "Failed to parse wallpapers for creative instance id "
                        << creative_instance_id);
            continue;
          }

          creative_set.creative_new_tab_page_ads.push_back(creative);
        } else if (code == "promoted_content_all_v1") {
          CatalogCreativePromotedContentAdInfo creative;

          creative.instance_id = creative_instance_id;

          // Type
          creative.type.code = code;
          creative.type.name = type["name"].GetString();
          creative.type.platform = type["platform"].GetString();
          creative.type.version = type["version"].GetInt();

          // Payload
          const auto& payload = creative_node["payload"].GetObject();
          creative.payload = CatalogPromotedContentAdPayloadInfo{
              .title = payload["title"].GetString(),
              .description = payload["description"].GetString(),
              .target_url = GURL(payload["feed"].GetString())};
          if (!ShouldSupportUrl(creative.payload.target_url)) {
            BLOG(1, "Target URL for creative instance id "
                        << creative_instance_id << " is unsupported");
            continue;
          }

          creative_set.conversions.erase(
              base::ranges::remove_if(
                  creative_set.conversions,
                  [&creative_set,
                   &creative](const CatalogConversionInfo& conversion) {
                    const GURL conversion_url_pattern =
                        GURL(conversion.url_pattern);
                    return conversion.creative_set_id == creative_set.id &&
                           (!ShouldSupportUrl(conversion_url_pattern) ||
                            !SameDomainOrHost(creative.payload.target_url,
                                              conversion_url_pattern));
                  }),
              creative_set.conversions.cend());

          creative_set.creative_promoted_content_ads.push_back(creative);
        } else {
          // Unknown type
          continue;
        }
      }

      campaign.creative_sets.push_back(creative_set);
    }

    catalog.campaigns.push_back(campaign);
  }

  return catalog;
}

}  // namespace brave_ads::json::reader
