/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_set.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time_delta_from_string.h"
#include "base/types/cxx23_to_underlying.h"
#include "base/types/optional_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/common/url/url_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

constexpr std::string_view kUndefinedAdMetricType;
constexpr std::string_view kDisabledAdMetricType = "disabled";
constexpr std::string_view kConfirmationAdMetricType = "confirmation";
constexpr std::string_view kP3AAdMetricType = "p3a";

constexpr auto kStringToMojomMap =
    base::MakeFixedFlatMap<std::string_view, mojom::NewTabPageAdMetricType>(
        {{kUndefinedAdMetricType, mojom::NewTabPageAdMetricType::kUndefined},
         {kDisabledAdMetricType, mojom::NewTabPageAdMetricType::kDisabled},
         {kConfirmationAdMetricType,
          mojom::NewTabPageAdMetricType::kConfirmation},
         {kP3AAdMetricType, mojom::NewTabPageAdMetricType::kP3A}});

constexpr auto kMojomToStringMap =
    base::MakeFixedFlatMap<mojom::NewTabPageAdMetricType, std::string_view>({
        {mojom::NewTabPageAdMetricType::kUndefined, kUndefinedAdMetricType},
        {mojom::NewTabPageAdMetricType::kDisabled, kDisabledAdMetricType},
        {mojom::NewTabPageAdMetricType::kConfirmation,
         kConfirmationAdMetricType},
        {mojom::NewTabPageAdMetricType::kP3A, kP3AAdMetricType},
    });

// Schema keys.
constexpr int kExpectedSchemaVersion = 2;
constexpr char kSchemaVersionKey[] = "schemaVersion";

// Grace period keys.
constexpr char kGracePeriodKey[] = "gracePeriod";

// Campaign keys.
constexpr int kExpectedCampaignVersion = 1;

constexpr char kCampaignsKey[] = "campaigns";
constexpr char kCampaignVersionKey[] = "version";
constexpr char kCampaignIdKey[] = "campaignId";

constexpr char kCampaignAdvertiserIdKey[] = "advertiserId";

constexpr char kCampaignMetricsKey[] = "metrics";

constexpr char kCampaignStartAtKey[] = "startAt";
constexpr char kCampaignEndAtKey[] = "endAt";

constexpr char kCampaignDailyCapKey[] = "dailyCap";

constexpr char kCampaignPriorityKey[] = "priority";
constexpr char kCampaignPassThroughRateKey[] = "ptr";

constexpr char kCampaignGeoTargetsKey[] = "geoTargets";

constexpr char kCampaignDayPartsKey[] = "dayParts";
constexpr char kCampaignDayPartDaysOfWeekKey[] = "daysOfWeek";
constexpr char kCampaignDayPartStartMinuteKey[] = "startMinute";
constexpr char kCampaignDayPartEndMinuteKey[] = "endMinute";

// Creative set keys.
constexpr char kCreativeSetsKey[] = "creativeSets";
constexpr char kCreativeSetIdKey[] = "creativeSetId";

constexpr char kCreativeSetPerDayKey[] = "perDay";
constexpr char kCreativeSetPerWeekKey[] = "perWeek";
constexpr char kCreativeSetPerMonthKey[] = "perMonth";
constexpr char kCreativeSetTotalMaxKey[] = "totalMax";

constexpr char kCreativeSetValueKey[] = "value";

constexpr char kCreativeSetSegmentsKey[] = "segments";

constexpr char kCreativeSetSplitTestGroupKey[] = "splitTestGroup";

constexpr char kCreativeSetConversionsKey[] = "conversions";
constexpr char kCreativeSetConversionUrlPatternKey[] = "urlPattern";
constexpr char kCreativeSetConversionObservationWindowKey[] =
    "observationWindow";
constexpr char kCreativeSetConversionPublicKeyKey[] = "publicKey";

// Creative keys.
constexpr char kCreativesKey[] = "creatives";
constexpr char kCreativeInstanceIdKey[] = "creativeInstanceId";

constexpr char kCreativeWallpaperKey[] = "wallpaper";
constexpr char kCreativeWallpaperTypeKey[] = "type";

constexpr char kCreativeCompanyNameKey[] = "companyName";
constexpr char kCreativeAltKey[] = "alt";

constexpr char kCreativeTargetUrlKey[] = "targetUrl";

constexpr char kCreativeConditionMatchersKey[] = "conditionMatchers";
constexpr char kCreativeConditionMatcherConditionKey[] = "condition";
constexpr char kCreativeConditionMatcherPrefPathKey[] = "prefPath";

void SaveCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions,
    ResultCallback callback) {
  database::table::CreativeSetConversions database_table;
  database_table.Save(
      creative_set_conversions,
      base::BindOnce(
          [](ResultCallback callback, bool success) {
            if (!success) {
              BLOG(0, "Failed to save creative set conversions");
              return std::move(callback).Run(/*success=*/false);
            }
            BLOG(0, "Successfully saved creative set conversions");

            std::move(callback).Run(/*success=*/true);
          },
          std::move(callback)));
}

void SaveCreativeNewTabPageAdsCallback(
    const CreativeSetConversionList& creative_set_conversions,
    ResultCallback callback,
    bool success) {
  if (!success) {
    BLOG(0, "Failed to save creative new tab page ads");
    return std::move(callback).Run(/*success=*/false);
  }
  BLOG(0, "Successfully saved creative new tab page ads");

  SaveCreativeSetConversions(creative_set_conversions, std::move(callback));
}

}  // namespace

void ParseAndSaveNewTabPageAds(base::Value::Dict dict,
                               ResultCallback callback) {
  std::optional<int> schema_version = dict.FindInt(kSchemaVersionKey);
  if (schema_version != kExpectedSchemaVersion) {
    // Currently, only version 2 is supported. Update this code to maintain.
    return std::move(callback).Run(/*success=*/false);
  }

  base::TimeDelta grace_period = base::Days(3);
  if (const std::string* const value = dict.FindString(kGracePeriodKey)) {
    grace_period = base::TimeDeltaFromString(*value).value_or(grace_period);
    BLOG(1, "Grace period changed to " << grace_period);
  } else {
    BLOG(1, "Grace period not specified, defaulting to " << grace_period);
  }
  SetProfileTimeDeltaPref(prefs::kGracePeriod, grace_period);

  const base::Value::List* const campaign_list = dict.FindList(kCampaignsKey);
  if (!campaign_list) {
    BLOG(0, "Campaigns are required");
    return std::move(callback).Run(/*success=*/false);
  }

  CreativeNewTabPageAdList creative_ads;
  CreativeSetConversionList creative_set_conversions;

  // Campaigns.
  for (const auto& campaign_value : *campaign_list) {
    const base::Value::Dict* const campaign_dict = campaign_value.GetIfDict();
    if (!campaign_dict) {
      BLOG(0, "Malformed campaign, skipping campaign");
      continue;
    }

    std::optional<int> campaign_version =
        campaign_dict->FindInt(kCampaignVersionKey);
    if (campaign_version != kExpectedCampaignVersion) {
      // Currently, only version 1 is supported. Update this code to maintain
      // backwards compatibility when adding new schema versions.
      continue;
    }

    const std::string* const campaign_id =
        campaign_dict->FindString(kCampaignIdKey);
    if (!campaign_id) {
      BLOG(0, "Campaign ID is required, skipping campaign");
      continue;
    }

    mojom::NewTabPageAdMetricType mojom_ad_metric_type =
        mojom::NewTabPageAdMetricType::kConfirmation;
    if (const std::string* const value =
            campaign_dict->FindString(kCampaignMetricsKey)) {
      mojom_ad_metric_type = ToMojomNewTabPageAdMetricType(*value).value_or(
          mojom::NewTabPageAdMetricType::kConfirmation);
    }

    const std::string* const advertiser_id =
        campaign_dict->FindString(kCampaignAdvertiserIdKey);
    if (!advertiser_id) {
      BLOG(0, "Advertiser ID is required, skipping campaign");
      continue;
    }

    base::Time start_at;
    if (const std::string* const value =
            campaign_dict->FindString(kCampaignStartAtKey)) {
      // Start at is optional.
      if (!base::Time::FromUTCString(value->c_str(), &start_at)) {
        BLOG(0, "Failed to parse campaign start at, skipping campaign");
        continue;
      }
    } else {
      // Default to starting immediately.
      start_at = base::Time::Now();
    }

    base::Time end_at;
    if (const std::string* const value =
            campaign_dict->FindString(kCampaignEndAtKey)) {
      // End at is optional.
      if (!base::Time::FromUTCString(value->c_str(), &end_at)) {
        BLOG(0, "Failed to parse campaign end at, skipping campaign");
        continue;
      }
    } else {
      // Default to running indefinitely.
      end_at = base::Time::Max();
    }

    const int daily_cap =
        campaign_dict->FindInt(kCampaignDailyCapKey).value_or(0);

    const int priority =
        campaign_dict->FindInt(kCampaignPriorityKey).value_or(10);

    const double pass_through_rate =
        campaign_dict->FindDouble(kCampaignPassThroughRateKey).value_or(1.0);

    // Geo targets.
    const base::Value::List* const geo_target_list =
        campaign_dict->FindList(kCampaignGeoTargetsKey);
    if (!geo_target_list || geo_target_list->empty()) {
      BLOG(0, "Geo targets are required, skipping campaign");
      continue;
    }

    base::flat_set<std::string> geo_targets;
    for (const auto& geo_target_value : *geo_target_list) {
      const std::string* const geo_target = geo_target_value.GetIfString();
      if (!geo_target) {
        BLOG(0, "Malformed geo target, skipping geo target");
        continue;
      }

      geo_targets.insert(*geo_target);
    }

    // Dayparts.
    CreativeDaypartSet dayparts;
    if (const base::Value::List* const list =
            campaign_dict->FindList(kCampaignDayPartsKey)) {
      // Dayparts are optional.
      for (const auto& value : *list) {
        const base::Value::Dict* const daypart_dict = value.GetIfDict();
        if (!daypart_dict) {
          BLOG(0, "Malformed daypart, skipping campaign");
          continue;
        }

        const std::string* const days_of_week =
            daypart_dict->FindString(kCampaignDayPartDaysOfWeekKey);
        if (!days_of_week) {
          BLOG(0, "Days of week is required, skipping campaign");
          continue;
        }

        const int start_minute =
            daypart_dict->FindInt(kCampaignDayPartStartMinuteKey)
                .value_or(0 /*00:00*/);

        const int end_minute =
            daypart_dict->FindInt(kCampaignDayPartEndMinuteKey)
                .value_or(1439 /*23:59*/);

        dayparts.insert(
            CreativeDaypartInfo{*days_of_week, start_minute, end_minute});
      }
    }
    if (dayparts.empty()) {
      // Default to all day every day.
      CreativeDaypartInfo daypart;
      dayparts.insert(daypart);
    }

    // Creative sets.
    const base::Value::List* const creative_set_list =
        campaign_dict->FindList(kCreativeSetsKey);
    if (!creative_set_list) {
      BLOG(0, "Creative sets are required, skipping campaign");
      continue;
    }

    for (const auto& creative_set_value : *creative_set_list) {
      const base::Value::Dict* const creative_set_dict =
          creative_set_value.GetIfDict();
      if (!creative_set_dict) {
        BLOG(0, "Malformed creative set, skipping creative set");
        continue;
      }

      CreativeNewTabPageAdInfo creative_ad;
      creative_ad.campaign_id = *campaign_id;
      creative_ad.advertiser_id = *advertiser_id;
      creative_ad.metric_type = mojom_ad_metric_type;
      creative_ad.start_at = start_at;
      creative_ad.end_at = end_at;
      creative_ad.daily_cap = daily_cap;
      creative_ad.priority = priority;
      creative_ad.pass_through_rate = pass_through_rate;
      creative_ad.geo_targets = geo_targets;
      creative_ad.dayparts = dayparts;

      // Creative set.
      const std::string* const creative_set_id =
          creative_set_dict->FindString(kCreativeSetIdKey);
      if (!creative_set_id) {
        BLOG(0, "Creative set ID is required, skipping creative set");
        continue;
      }
      creative_ad.creative_set_id = *creative_set_id;

      creative_ad.per_day =
          creative_set_dict->FindInt(kCreativeSetPerDayKey).value_or(0);
      creative_ad.per_week =
          creative_set_dict->FindInt(kCreativeSetPerWeekKey).value_or(0);
      creative_ad.per_month =
          creative_set_dict->FindInt(kCreativeSetPerMonthKey).value_or(0);
      creative_ad.total_max =
          creative_set_dict->FindInt(kCreativeSetTotalMaxKey).value_or(0);

      if (const std::string* const value =
              creative_set_dict->FindString(kCreativeSetValueKey)) {
        // Value is optional.
        if (!base::StringToDouble(*value, &creative_ad.value)) {
          BLOG(0, "Failed to parse associated value, skipping creative set");
          continue;
        }
      } else {
        // Default to zero value.
        creative_ad.value = 0.0;
      }

      // Split test group.
      if (const std::string* const value =
              creative_set_dict->FindString(kCreativeSetSplitTestGroupKey)) {
        // Split test group is optional.
        creative_ad.split_test_group = *value;
      }

      // Conversions.
      const base::Value::List* const conversion_list =
          creative_set_dict->FindList(kCreativeSetConversionsKey);
      if (conversion_list) {
        // Conversions are optional.
        for (const auto& conversion_value : *conversion_list) {
          const base::Value::Dict* const conversion_dict =
              conversion_value.GetIfDict();
          if (!conversion_dict) {
            BLOG(0, "Malformed conversion, skipping conversion");
            continue;
          }

          CreativeSetConversionInfo creative_set_conversion;

          creative_set_conversion.id = creative_ad.creative_set_id;

          const std::string* const url_pattern =
              conversion_dict->FindString(kCreativeSetConversionUrlPatternKey);
          if (!url_pattern) {
            // URL pattern is required.
            BLOG(0,
                 "URL pattern is required, skipping creative set conversion");
            continue;
          }
          creative_set_conversion.url_pattern = *url_pattern;

          const int observation_window =
              conversion_dict
                  ->FindInt(kCreativeSetConversionObservationWindowKey)
                  .value_or(7);
          creative_set_conversion.observation_window =
              base::Days(observation_window);

          creative_set_conversion.expire_at =
              creative_ad.end_at + creative_set_conversion.observation_window;

          const std::string* const public_key =
              conversion_dict->FindString(kCreativeSetConversionPublicKeyKey);
          if (public_key) {
            creative_set_conversion.verifiable_advertiser_public_key_base64 =
                *public_key;
          }

          creative_set_conversions.push_back(creative_set_conversion);
        }
      }

      // Segments.
      SegmentList segments;
      if (const base::Value::List* const list =
              creative_set_dict->FindList(kCreativeSetSegmentsKey)) {
        // Segments are optional.
        for (const auto& value : *list) {
          const std::string* const segment = value.GetIfString();
          if (!segment) {
            BLOG(0, "Malformed segment, skipping segment");
            continue;
          }

          segments.push_back(*segment);
        }
      }
      if (segments.empty()) {
        // Default to untargeted segment.
        segments.emplace_back(kUntargetedSegment);
      }

      // Creatives.
      const base::Value::List* const creative_list =
          creative_set_dict->FindList(kCreativesKey);
      if (!creative_list) {
        BLOG(0, "Creatives are required, skipping creative set");
        continue;
      }

      for (const auto& creative_value : *creative_list) {
        const base::Value::Dict* const creative_dict =
            creative_value.GetIfDict();
        if (!creative_dict) {
          BLOG(0, "Malformed creative, skipping creative");
          continue;
        }

        // Creative.
        const std::string* const creative_instance_id =
            creative_dict->FindString(kCreativeInstanceIdKey);
        if (!creative_instance_id) {
          BLOG(0, "Creative instance ID is required, skipping creative");
          continue;
        }
        creative_ad.creative_instance_id = *creative_instance_id;

        const std::string* const company_name =
            creative_dict->FindString(kCreativeCompanyNameKey);
        if (!company_name) {
          BLOG(0, "Company name is required, skipping creative");
          continue;
        }
        creative_ad.company_name = *company_name;

        const std::string* const alt =
            creative_dict->FindString(kCreativeAltKey);
        if (!alt) {
          BLOG(0, "Alt is required, skipping creative");
          continue;
        }
        creative_ad.alt = *alt;

        const std::string* const target_url =
            creative_dict->FindString(kCreativeTargetUrlKey);
        if (!target_url) {
          BLOG(0, "Target URL is required, skipping creative");
          continue;
        }
        creative_ad.target_url = GURL(*target_url);
        if (!ShouldSupportUrl(creative_ad.target_url)) {
          BLOG(0, "Invalid target URL, skipping creative");
          continue;
        }

        // Wallpaper.
        const base::Value::Dict* const wallpaper_dict =
            creative_dict->FindDict(kCreativeWallpaperKey);
        if (!wallpaper_dict) {
          BLOG(0, "Wallpaper is required, skipping creative");
          continue;
        }

        const std::string* const wallpaper_type =
            wallpaper_dict->FindString(kCreativeWallpaperTypeKey);
        if (!wallpaper_type) {
          BLOG(0, "Wallpaper type is required, skipping creative");
          continue;
        }
        if (*wallpaper_type != kCreativeNewTabPageAdImageWallpaperType &&
            *wallpaper_type != kCreativeNewTabPageAdRichMediaWallpaperType) {
          BLOG(0, "Unknown wallpaper type, skipping creative");
          continue;
        }
        creative_ad.wallpaper_type =
            ToCreativeNewTabPageAdWallpaperType(*wallpaper_type);

        // Condition matchers.
        const base::Value::List* const condition_matcher_list =
            creative_dict->FindList(kCreativeConditionMatchersKey);
        if (condition_matcher_list) {
          // Condition matchers are optional.
          ConditionMatcherMap condition_matchers;
          for (const auto& condition_matcher_value : *condition_matcher_list) {
            const base::Value::Dict* const condition_matcher_dict =
                condition_matcher_value.GetIfDict();
            if (!condition_matcher_dict) {
              BLOG(0,
                   "Malformed condition matcher, skipping condition matcher");
              continue;
            }

            const std::string* const condition =
                condition_matcher_dict->FindString(
                    kCreativeConditionMatcherConditionKey);
            if (!condition) {
              BLOG(0, "Condition is required, skipping condition matcher");
              continue;
            }

            const std::string* const pref_path =
                condition_matcher_dict->FindString(
                    kCreativeConditionMatcherPrefPathKey);
            if (!pref_path) {
              BLOG(0, "Pref path is required, skipping condition matcher");
              continue;
            }

            condition_matchers.emplace(*pref_path, *condition);
          }

          creative_ad.condition_matchers = condition_matchers;
        }

        for (const auto& segment : segments) {
          creative_ad.segment = segment;
          creative_ads.push_back(creative_ad);

          UpdateReportMetricState(creative_ad.creative_instance_id,
                                  mojom_ad_metric_type);
        }
      }
    }
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.Save(
      creative_ads,
      base::BindOnce(&SaveCreativeNewTabPageAdsCallback,
                     creative_set_conversions, std::move(callback)));
}

std::optional<mojom::NewTabPageAdMetricType> ToMojomNewTabPageAdMetricType(
    std::string_view value) {
  return base::OptionalFromPtr(base::FindOrNull(kStringToMojomMap, value));
}

std::string_view ToString(mojom::NewTabPageAdMetricType value) {
  const auto iter = kMojomToStringMap.find(value);
  if (iter != kMojomToStringMap.cend()) {
    return iter->second;
  }

  NOTREACHED() << "Unexpected value for mojom::NewTabPageAdMetricType: "
               << base::to_underlying(value);
}

}  // namespace brave_ads
