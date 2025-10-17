/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_json_reader.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_campaign_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_daypart_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_geo_target_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_conversion_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_creative_set_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_os_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_segment_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/catalog_type_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/common/url/url_util.h"
#include "url/gurl.h"

namespace brave_ads::json::reader {

namespace {

constexpr base::TimeDelta kDefaultCatalogPing = base::Days(2);

constexpr int kDefaultCampaignPriority = 1;
constexpr double kDefaultCampaignPassThroughRate = 1.0;
constexpr int kDefaultCampaignDailyCap = 0;

constexpr int kMinCampaignDaypartStartMinute = 0;   // Midnight.
constexpr int kMaxCampaignDaypartEndMinute = 1439;  // End of day.

constexpr int kDefaultCreativeSetPerDay = 0;
constexpr int kDefaultCreativeSetPerWeek = 0;
constexpr int kDefaultCreativeSetPerMonth = 0;
constexpr int kDefaultCreativeSetTotalMax = 0;

constexpr int kDefaultConversionObservationWindow = 7;

template <typename T>
std::optional<std::vector<T>> ParseListWithCodeAndName(
    const base::Value::Dict& dict,
    const std::string& key) {
  const base::Value::List* const list = dict.FindList(key);
  if (!list) {
    return std::nullopt;
  }

  std::vector<T> items;
  items.reserve(list->size());

  for (const auto& value : *list) {
    const base::Value::Dict* const item_dict = value.GetIfDict();
    if (!item_dict) {
      BLOG(1, "Invalid " << key);
      return std::nullopt;
    }

    const std::string* const code = item_dict->FindString("code");
    if (!code || code->empty()) {
      BLOG(1, "Invalid " << key << " code");
      return std::nullopt;
    }

    const std::string* const name = item_dict->FindString("name");
    if (!name || name->empty()) {
      BLOG(1, "Invalid " << key << " name");
      return std::nullopt;
    }

    items.push_back({.code = *code, .name = *name});
  }

  return items;
}

std::optional<CatalogGeoTargetList> ParseGeoTargets(
    const base::Value::Dict& dict) {
  return ParseListWithCodeAndName<CatalogGeoTargetInfo>(dict, "geoTargets");
}

std::optional<CatalogDaypartList> ParseDayparts(const base::Value::Dict& dict) {
  CatalogDaypartList dayparts;

  const base::Value::List* const list = dict.FindList("dayParts");
  if (!list || list->empty()) {
    // Fallback to 24/7.
    dayparts.push_back(CatalogDaypartInfo{});
    return dayparts;
  }

  dayparts.reserve(list->size());

  for (const auto& value : *list) {
    const base::Value::Dict* const daypart_dict = value.GetIfDict();
    if (!daypart_dict) {
      BLOG(1, "Invalid daypart");
      return std::nullopt;
    }

    CatalogDaypartInfo daypart;

    const std::string* const days_of_week = daypart_dict->FindString("dow");
    if (!days_of_week || days_of_week->empty()) {
      BLOG(1, "Invalid daypart days of week");
      return std::nullopt;
    }
    daypart.days_of_week = *days_of_week;

    std::optional<int> start_minute = daypart_dict->FindInt("startMinute");
    if (!start_minute || start_minute < kMinCampaignDaypartStartMinute) {
      BLOG(1, "Invalid daypart start minute");
      return std::nullopt;
    }
    daypart.start_minute = *start_minute;

    std::optional<int> end_minute = daypart_dict->FindInt("endMinute");
    if (!end_minute || end_minute > kMaxCampaignDaypartEndMinute) {
      BLOG(1, "Invalid daypart end minute");
      return std::nullopt;
    }
    daypart.end_minute = *end_minute;

    dayparts.push_back(std::move(daypart));
  }

  return dayparts;
}

std::optional<CatalogSegmentList> ParseSegments(const base::Value::Dict& dict) {
  return ParseListWithCodeAndName<CatalogSegmentInfo>(dict, "segments");
}

std::optional<CatalogOsList> ParseOses(const base::Value::Dict& dict) {
  return ParseListWithCodeAndName<CatalogOsInfo>(dict, "oses");
}

std::optional<CatalogConversionList> ParseConversions(
    const base::Value::Dict& dict,
    const CatalogCampaignInfo& campaign,
    const CatalogCreativeSetInfo& creative_set) {
  const base::Value::List* const list = dict.FindList("conversions");
  if (!list) {
    return std::nullopt;
  }

  CatalogConversionList conversions;
  conversions.reserve(list->size());

  for (const auto& value : *list) {
    const base::Value::Dict* const conversion_dict = value.GetIfDict();
    if (!conversion_dict) {
      BLOG(1, "Invalid conversion");
      return std::nullopt;
    }

    CatalogConversionInfo conversion;
    conversion.creative_set_id = creative_set.id;

    const std::string* const url_pattern =
        conversion_dict->FindString("urlPattern");
    if (!url_pattern || url_pattern->empty()) {
      BLOG(1, "Invalid conversion URL pattern");
      return std::nullopt;
    }
    // Invalid conversions are filtered later by `FilterInvalidConversions`.
    conversion.url_pattern = *url_pattern;

    if (const std::string* const public_key =
            conversion_dict->FindString("conversionPublicKey")) {
      // Optional.
      conversion.verifiable_advertiser_public_key_base64 = *public_key;
    }

    conversion.observation_window =
        base::Days(conversion_dict->FindInt("observationWindow")
                       .value_or(kDefaultConversionObservationWindow));

    conversion.expire_at = campaign.end_at + conversion.observation_window;

    conversions.push_back(std::move(conversion));
  }

  return conversions;
}

std::optional<CatalogCreativeNotificationAdInfo> ParseCreativeNotificationAd(
    const base::Value::Dict& dict) {
  CatalogCreativeNotificationAdInfo creative;

  const std::string* const title = dict.FindString("title");
  if (!title || title->empty()) {
    BLOG(1, "Invalid notification ad title");
    return std::nullopt;
  }
  creative.payload.title = *title;

  const std::string* const body = dict.FindString("body");
  if (!body || body->empty()) {
    BLOG(1, "Invalid notification ad body");
    return std::nullopt;
  }
  creative.payload.body = *body;

  const std::string* const target_url = dict.FindString("targetUrl");
  if (!target_url || target_url->empty()) {
    BLOG(1, "Invalid notification ad target URL");
    return std::nullopt;
  }
  creative.payload.target_url = GURL(*target_url);
  if (!ShouldSupportUrl(creative.payload.target_url)) {
    BLOG(1, "Unsupported notification ad target URL");
    return std::nullopt;
  }

  return creative;
}

std::optional<CatalogCreativeInlineContentAdInfo> ParseCreativeInlineContentAd(
    const base::Value::Dict& dict) {
  CatalogCreativeInlineContentAdInfo creative;

  const std::string* const title = dict.FindString("title");
  if (!title || title->empty()) {
    BLOG(1, "Invalid inline content ad title");
    return std::nullopt;
  }
  creative.payload.title = *title;

  const std::string* const description = dict.FindString("description");
  if (!description || description->empty()) {
    BLOG(1, "Invalid inline content ad description");
    return std::nullopt;
  }
  creative.payload.description = *description;

  const std::string* const image_url = dict.FindString("imageUrl");
  if (!image_url || image_url->empty()) {
    BLOG(1, "Invalid inline content ad image URL");
    return std::nullopt;
  }
  creative.payload.image_url = GURL(*image_url);
  if (!ShouldSupportUrl(creative.payload.image_url)) {
    BLOG(1, "Unsupported inline content ad image URL");
    return std::nullopt;
  }

  const std::string* const dimensions = dict.FindString("dimensions");
  if (!dimensions || dimensions->empty()) {
    BLOG(1, "Invalid inline content ad dimensions");
    return std::nullopt;
  }
  creative.payload.dimensions = *dimensions;

  const std::string* const cta_text = dict.FindString("ctaText");
  if (!cta_text || cta_text->empty()) {
    BLOG(1, "Invalid inline content ad CTA text");
    return std::nullopt;
  }
  creative.payload.cta_text = *cta_text;

  const std::string* const target_url = dict.FindString("targetUrl");
  if (!target_url || target_url->empty()) {
    BLOG(1, "Invalid inline content ad target URL");
    return std::nullopt;
  }
  creative.payload.target_url = GURL(*target_url);
  if (!ShouldSupportUrl(creative.payload.target_url)) {
    BLOG(1, "Unsupported inline content ad target URL");
    return std::nullopt;
  }

  return creative;
}

std::optional<CatalogCreativePromotedContentAdInfo>
ParseCreativePromotedContentAd(const base::Value::Dict& dict) {
  CatalogCreativePromotedContentAdInfo creative;

  const std::string* const title = dict.FindString("title");
  if (!title || title->empty()) {
    BLOG(1, "Invalid promoted content ad title");
    return std::nullopt;
  }
  creative.payload.title = *title;

  const std::string* const description = dict.FindString("description");
  if (!description || description->empty()) {
    BLOG(1, "Invalid promoted content ad description");
    return std::nullopt;
  }
  creative.payload.description = *description;

  const std::string* const feed = dict.FindString("feed");
  if (!feed || feed->empty()) {
    BLOG(1, "Invalid promoted content ad feed URL");
    return std::nullopt;
  }
  creative.payload.target_url = GURL(*feed);
  if (!ShouldSupportUrl(creative.payload.target_url)) {
    BLOG(1, "Unsupported promoted content ad feed URL");
    return std::nullopt;
  }

  return creative;
}

std::optional<CatalogTypeInfo> ParseCreativeType(
    const base::Value::Dict& dict) {
  const base::Value::Dict* const type_dict = dict.FindDict("type");
  if (!type_dict) {
    BLOG(1, "Invalid creative type");
    return std::nullopt;
  }

  CatalogTypeInfo type;

  const std::string* const code = type_dict->FindString("code");
  if (!code || code->empty()) {
    BLOG(1, "Invalid creative type code");
    return std::nullopt;
  }
  type.code = *code;

  const std::string* const name = type_dict->FindString("name");
  if (!name || name->empty()) {
    BLOG(1, "Invalid creative type name");
    return std::nullopt;
  }
  type.name = *name;

  const std::string* const platform = type_dict->FindString("platform");
  if (!platform || platform->empty()) {
    BLOG(1, "Invalid creative type platform");
    return std::nullopt;
  }
  type.platform = *platform;

  std::optional<int> version = type_dict->FindInt("version");
  if (!version) {
    BLOG(1, "Invalid creative type version");
    return std::nullopt;
  }
  type.version = *version;

  return type;
}

void FilterInvalidConversions(CatalogCreativeSetInfo& creative_set,
                              const GURL& target_url) {
  std::erase_if(
      creative_set.conversions,
      [&creative_set, &target_url](const CatalogConversionInfo& conversion) {
        if (conversion.creative_set_id != creative_set.id) {
          return false;
        }

        const GURL url_pattern(conversion.url_pattern);

        const bool is_valid = SameDomainOrHost(target_url, url_pattern) &&
                              ShouldSupportUrl(url_pattern);

        return !is_valid;
      });
}

template <typename CatalogCreativeAdInfo,
          typename ParseCreativeAdFunc,
          typename CreativeList>
bool ParseCreativeForType(ParseCreativeAdFunc parse_creative_ad_func,
                          CatalogCreativeSetInfo& creative_set,
                          const std::string& creative_instance_id,
                          const base::Value::Dict& payload_dict,
                          const CatalogTypeInfo& type,
                          CreativeList& creatives) {
  std::optional<CatalogCreativeAdInfo> creative =
      parse_creative_ad_func(payload_dict);
  if (!creative) {
    BLOG(1, "Invalid creative for type: " << type.code);
    return false;
  }

  creative->instance_id = creative_instance_id;

  creative->type = type;

  FilterInvalidConversions(creative_set, creative->payload.target_url);

  creatives.push_back(std::move(*creative));

  return true;
}

bool ParseCreative(const base::Value::Dict& dict,
                   CatalogCreativeSetInfo& creative_set) {
  const std::string* const creative_instance_id =
      dict.FindString("creativeInstanceId");
  if (!creative_instance_id || creative_instance_id->empty()) {
    BLOG(1, "Invalid creative instance id");
    return false;
  }

  std::optional<CatalogTypeInfo> type = ParseCreativeType(dict);
  if (!type) {
    BLOG(1, "Invalid creative type");
    return false;
  }

  const base::Value::Dict* const payload_dict = dict.FindDict("payload");
  if (!payload_dict) {
    BLOG(1, "Invalid creative payload");
    return false;
  }

  if (type->code == "notification_all_v1") {
    return ParseCreativeForType<CatalogCreativeNotificationAdInfo>(
        ParseCreativeNotificationAd, creative_set, *creative_instance_id,
        *payload_dict, *type, creative_set.creative_notification_ads);
  }

  if (type->code == "inline_content_all_v1") {
    return ParseCreativeForType<CatalogCreativeInlineContentAdInfo>(
        ParseCreativeInlineContentAd, creative_set, *creative_instance_id,
        *payload_dict, *type, creative_set.creative_inline_content_ads);
  }

  if (type->code == "promoted_content_all_v1") {
    return ParseCreativeForType<CatalogCreativePromotedContentAdInfo>(
        ParseCreativePromotedContentAd, creative_set, *creative_instance_id,
        *payload_dict, *type, creative_set.creative_promoted_content_ads);
  }

  // Unsupported type, ignore it to avoid failing the entire catalog.
  return true;
}

bool ParseCreatives(const base::Value::Dict& dict,
                    CatalogCreativeSetInfo& creative_set) {
  const base::Value::List* const list = dict.FindList("creatives");
  if (!list) {
    return false;
  }

  for (const auto& value : *list) {
    const base::Value::Dict* const creative_dict = value.GetIfDict();
    if (!creative_dict) {
      BLOG(1, "Invalid creative");
      return false;
    }

    if (!ParseCreative(*creative_dict, creative_set)) {
      BLOG(1,
           "Failed to parse creative for creative set id " << creative_set.id);
      return false;
    }
  }

  return true;
}

std::optional<CatalogCreativeSetInfo> ParseCreativeSet(
    const base::Value::Dict& dict,
    const CatalogCampaignInfo& campaign) {
  CatalogCreativeSetInfo creative_set;

  const std::string* const creative_set_id = dict.FindString("creativeSetId");
  if (!creative_set_id || creative_set_id->empty()) {
    BLOG(1, "Invalid creative set id");
    return std::nullopt;
  }
  creative_set.id = *creative_set_id;

  creative_set.per_day =
      dict.FindInt("perDay").value_or(kDefaultCreativeSetPerDay);
  creative_set.per_week =
      dict.FindInt("perWeek").value_or(kDefaultCreativeSetPerWeek);
  creative_set.per_month =
      dict.FindInt("perMonth").value_or(kDefaultCreativeSetPerMonth);

  creative_set.total_max =
      dict.FindInt("totalMax").value_or(kDefaultCreativeSetTotalMax);

  const std::string* const associated_value = dict.FindString("value");
  if (!associated_value ||
      !base::StringToDouble(*associated_value, &creative_set.value)) {
    BLOG(1, "Invalid associated value");
    return std::nullopt;
  }

  if (const std::string* const split_group =
          dict.FindString("splitTestGroup")) {
    // Optional.
    creative_set.split_test_group = *split_group;
  }

  std::optional<CatalogSegmentList> segments = ParseSegments(dict);
  if (!segments || segments->empty()) {
    // At least one segment is required.
    BLOG(1, "Invalid segments");
    return std::nullopt;
  }
  creative_set.segments = *segments;

  if (std::optional<CatalogOsList> oses = ParseOses(dict)) {
    // Optional.
    creative_set.oses = *oses;
  }

  if (std::optional<CatalogConversionList> conversions =
          ParseConversions(dict, campaign, creative_set)) {
    // Optional.
    creative_set.conversions = *conversions;
  }

  if (!ParseCreatives(dict, creative_set)) {
    // At least one creative is required.
    BLOG(1,
         "Failed to parse creatives for creative set id " << creative_set.id);
    return std::nullopt;
  }

  return creative_set;
}

std::optional<CatalogCreativeSetList> ParseCreativeSets(
    const base::Value::Dict& dict,
    const CatalogCampaignInfo& campaign) {
  const base::Value::List* const list = dict.FindList("creativeSets");
  if (!list) {
    return std::nullopt;
  }

  CatalogCreativeSetList creative_sets;
  creative_sets.reserve(list->size());

  for (const auto& value : *list) {
    const base::Value::Dict* const creative_set_dict = value.GetIfDict();
    if (!creative_set_dict) {
      BLOG(1, "Invalid creative set");
      continue;
    }

    if (std::optional<CatalogCreativeSetInfo> creative_set =
            ParseCreativeSet(*creative_set_dict, campaign)) {
      creative_sets.push_back(std::move(*creative_set));
    }
  }

  return creative_sets;
}

std::optional<CatalogCampaignInfo> ParseCampaign(
    const base::Value::Dict& dict) {
  CatalogCampaignInfo campaign;

  const std::string* const campaign_id = dict.FindString("campaignId");
  if (!campaign_id || campaign_id->empty()) {
    BLOG(1, "Invalid campaign id");
    return std::nullopt;
  }
  campaign.id = *campaign_id;

  campaign.priority =
      dict.FindInt("priority").value_or(kDefaultCampaignPriority);

  campaign.pass_through_rate =
      dict.FindDouble("ptr").value_or(kDefaultCampaignPassThroughRate);

  const std::string* const start_at = dict.FindString("startAt");
  if (!start_at || start_at->empty()) {
    BLOG(1, "Invalid campaign start at");
    return std::nullopt;
  }
  if (!base::Time::FromUTCString(start_at->c_str(), &campaign.start_at)) {
    BLOG(1, "Invalid campaign start at: " << *start_at);
    return std::nullopt;
  }

  const std::string* const end_at = dict.FindString("endAt");
  if (!end_at || end_at->empty()) {
    BLOG(1, "Invalid campaign end at");
    return std::nullopt;
  }
  if (!base::Time::FromUTCString(end_at->c_str(), &campaign.end_at)) {
    BLOG(1, "Invalid campaign end at: " << *end_at);
    return std::nullopt;
  }

  campaign.daily_cap =
      dict.FindInt("dailyCap").value_or(kDefaultCampaignDailyCap);

  const std::string* const advertiser_id = dict.FindString("advertiserId");
  if (!advertiser_id || advertiser_id->empty()) {
    BLOG(1, "Invalid advertiser id");
    return std::nullopt;
  }
  campaign.advertiser_id = *advertiser_id;

  std::optional<CatalogGeoTargetList> geo_targets = ParseGeoTargets(dict);
  if (!geo_targets || geo_targets->empty()) {
    // At least one geo target is required.
    BLOG(1, "Invalid geo targets");
    return std::nullopt;
  }
  campaign.geo_targets = *geo_targets;

  std::optional<CatalogDaypartList> dayparts = ParseDayparts(dict);
  if (!dayparts) {
    BLOG(1, "Invalid dayparts");
    return std::nullopt;
  }
  campaign.dayparts = *dayparts;

  std::optional<CatalogCreativeSetList> creative_sets =
      ParseCreativeSets(dict, campaign);
  if (!creative_sets || creative_sets->empty()) {
    // At least one creative set is required.
    BLOG(1, "Invalid creative sets");
    return std::nullopt;
  }
  campaign.creative_sets = *creative_sets;

  return campaign;
}

std::optional<CatalogCampaignList> ParseCampaigns(
    const base::Value::Dict& dict) {
  const base::Value::List* const list = dict.FindList("campaigns");
  if (!list) {
    BLOG(1, "Missing campaigns");
    return std::nullopt;
  }

  CatalogCampaignList campaigns;
  campaigns.reserve(list->size());

  for (const auto& value : *list) {
    const base::Value::Dict* const campaign_dict = value.GetIfDict();
    if (!campaign_dict) {
      BLOG(1, "Invalid campaign");
      continue;
    }

    std::optional<CatalogCampaignInfo> campaign = ParseCampaign(*campaign_dict);
    if (!campaign) {
      BLOG(1, "Invalid campaign");
      continue;
    }

    campaigns.push_back(std::move(*campaign));
  }

  return campaigns;
}

std::optional<CatalogInfo> ParseCatalog(const base::Value::Dict& dict) {
  CatalogInfo catalog;

  const std::string* const catalog_id = dict.FindString("catalogId");
  if (!catalog_id || catalog_id->empty()) {
    BLOG(1, "Invalid catalog id");
    return std::nullopt;
  }
  catalog.id = *catalog_id;

  std::optional<int> catalog_version = dict.FindInt("version");
  if (!catalog_version || catalog_version != kCatalogVersion) {
    BLOG(1, "Invalid catalog version");
    return std::nullopt;
  }
  catalog.version = *catalog_version;

  std::optional<int> ping = dict.FindInt("ping");
  catalog.ping = ping ? base::Milliseconds(*ping) : kDefaultCatalogPing;

  std::optional<CatalogCampaignList> campaigns = ParseCampaigns(dict);
  if (!campaigns) {
    BLOG(1, "Failed to parse campaigns");
    return std::nullopt;
  }
  catalog.campaigns = *campaigns;

  return catalog;
}

}  // namespace

std::optional<CatalogInfo> ReadCatalog(const std::string& json) {
  std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!dict) {
    BLOG(0, "Failed to read catalog JSON");
    return std::nullopt;
  }

  TRACE_EVENT_BEGIN(kTraceEventCategory,
                    "CatalogUrlRequestJsonReader::ParseCatalog");
  std::optional<CatalogInfo> catalog = ParseCatalog(*dict);
  if (!catalog) {
    BLOG(0, "Failed to parse catalog");
    TRACE_EVENT_END1(kTraceEventCategory,
                     "CatalogUrlRequestJsonReader::ParseCatalog", "success",
                     false);
    return std::nullopt;
  }

  TRACE_EVENT_END2(kTraceEventCategory,
                   "CatalogUrlRequestJsonReader::ParseCatalog", "id",
                   catalog->id, "success", true);
  return catalog;
}

}  // namespace brave_ads::json::reader
