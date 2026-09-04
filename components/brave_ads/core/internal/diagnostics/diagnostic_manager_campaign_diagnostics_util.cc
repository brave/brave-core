/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager_campaign_diagnostics_util.h"

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

std::string ToString(mojom::NewTabPageAdMetricType mojom_metric_type) {
  switch (mojom_metric_type) {
    case mojom::NewTabPageAdMetricType::kUndefined:
      return "Undefined";
    case mojom::NewTabPageAdMetricType::kDisabled:
      return "Disabled";
    case mojom::NewTabPageAdMetricType::kConfirmation:
      return "Confirmation";
  }
  NOTREACHED();
}

// `company_name`/`alt` only exist on `CreativeNewTabPageAdInfo`, and
// `title`/`body` only exist on `CreativeNotificationAdInfo`; other ad types
// leave them empty rather than forcing every `T` to carry fields specific to
// one ad format.
template <typename T>
std::string GetCompanyName(const T& /*creative_ad*/) {
  return std::string();
}
std::string GetCompanyName(const CreativeNewTabPageAdInfo& creative_ad) {
  return creative_ad.company_name;
}

template <typename T>
std::string GetAlt(const T& /*creative_ad*/) {
  return std::string();
}
std::string GetAlt(const CreativeNewTabPageAdInfo& creative_ad) {
  return creative_ad.alt;
}

template <typename T>
std::string GetTitle(const T& /*creative_ad*/) {
  return std::string();
}
std::string GetTitle(const CreativeNotificationAdInfo& creative_ad) {
  return creative_ad.title;
}

template <typename T>
std::string GetBody(const T& /*creative_ad*/) {
  return std::string();
}
std::string GetBody(const CreativeNotificationAdInfo& creative_ad) {
  return creative_ad.body;
}

std::string ToString(CreativeNewTabPageAdWallpaperType wallpaper_type) {
  switch (wallpaper_type) {
    case CreativeNewTabPageAdWallpaperType::kUndefined:
      return std::string();
    case CreativeNewTabPageAdWallpaperType::kImage:
      return "Static";
    case CreativeNewTabPageAdWallpaperType::kRichMedia:
      return "Dynamic";
  }
  NOTREACHED();
}

// `wallpaper_type` only exists on `CreativeNewTabPageAdInfo`; other ad types
// leave it undefined.
template <typename T>
CreativeNewTabPageAdWallpaperType GetWallpaperType(const T& /*creative_ad*/) {
  return CreativeNewTabPageAdWallpaperType::kUndefined;
}
CreativeNewTabPageAdWallpaperType GetWallpaperType(
    const CreativeNewTabPageAdInfo& creative_ad) {
  return creative_ad.wallpaper_type;
}

struct CreativeInfo {
  std::string target_url;
  std::string company_name;
  std::string alt;
  std::string title;
  std::string body;
  CreativeNewTabPageAdWallpaperType wallpaper_type =
      CreativeNewTabPageAdWallpaperType::kUndefined;
};

// A creative set's frequency caps are duplicated across every creative ad row
// flattened from it in the database, so only the first one seen per creative
// set id needs to be kept.
struct CreativeSetInfo {
  int per_day = 0;
  int per_week = 0;
  int per_month = 0;
  int total_max = 0;
  // A creative set is flattened into one creative ad row per targeted
  // segment (see `creatives_builder.cc`), so the distinct segments across
  // its rows are its targeting list.
  base::flat_set<std::string> segments;
  std::map<std::string, CreativeInfo> creatives_by_instance_id;
};

// Aggregates every creative ad belonging to the same campaign into a single
// row, nesting creative instances under the creative set that serves them,
// so the UI can render the actual Campaign > Creative Set > Creatives
// hierarchy rather than just distinct counts.
struct CampaignInfo {
  std::string advertiser_id;
  base::Time start_at;
  base::Time end_at;
  int daily_cap = 0;
  int priority = 0;
  double pass_through_rate = 0.0;
  std::string metric_type;
  std::map<std::string, CreativeSetInfo> creative_sets_by_id;
  // The catalog's own geo-targeting for this campaign, as opposed to the
  // device's own locale-derived country; shown on the Resources tab's
  // Catalog card since it's the more meaningful "what region is this data
  // for" answer.
  base::flat_set<std::string> geo_targets;
  // The windows of the week this campaign is eligible to serve in; shown on
  // the Day Parts tab.
  CreativeDaypartSet dayparts;
};

// Groups served impression timestamps by campaign/creative set ID in a
// single pass over `ad_events`, so `CountServedImpressions` below only ever
// scans the (much smaller) subset of events belonging to the campaign or
// creative set it was asked about, rather than the full event history once
// per frequency-cap window.
struct ServedImpressionTimestamps final {
  std::map<std::string, std::vector<base::Time>> by_campaign_id;
  std::map<std::string, std::vector<base::Time>> by_creative_set_id;
};

ServedImpressionTimestamps BuildServedImpressionTimestamps(
    const AdEventList& ad_events) {
  ServedImpressionTimestamps timestamps;

  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type !=
            mojom::ConfirmationType::kServedImpression ||
        !ad_event.created_at) {
      continue;
    }

    timestamps.by_campaign_id[ad_event.campaign_id].push_back(
        *ad_event.created_at);
    timestamps.by_creative_set_id[ad_event.creative_set_id].push_back(
        *ad_event.created_at);
  }

  return timestamps;
}

// Mirrors the "served" half of `DoesRespectCampaignCap`/
// `DoesRespectCreativeSetCap` (see `exclusion_rule_util.cc`), without the
// early-exit optimization those need for serving but diagnostics reporting
// does not. `timestamps` is null when the campaign/creative set has no
// served impressions at all.
int CountServedImpressions(const std::vector<base::Time>* timestamps,
                           std::optional<base::TimeDelta> time_constraint) {
  if (!timestamps) {
    return 0;
  }

  if (!time_constraint) {
    return static_cast<int>(timestamps->size());
  }

  const base::Time now = base::Time::Now();
  return static_cast<int>(
      std::ranges::count_if(*timestamps, [&](base::Time created_at) {
        return now - created_at < *time_constraint;
      }));
}

// Builds the campaigns list and counts the active, region-targeted creative
// ads in the same pass, so callers needing both don't have to query the same
// table and re-apply `SubdivisionTargetingExclusionRule` twice.
template <typename T>
void BuildCampaigns(GetCampaignsDiagnosticsCallback callback,
                    const AdEventList& ad_events,
                    bool success,
                    const SegmentList& /*segments*/,
                    const std::vector<T>& creative_ads) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false, /*active_ad_count=*/0,
                                   base::ListValue());
  }

  const SubdivisionTargetingExclusionRule exclusion_rule(
      GetAdHandler().GetSubdivisionTargeting());

  const ServedImpressionTimestamps served_impression_timestamps =
      BuildServedImpressionTimestamps(ad_events);

  size_t active_ad_count = 0;
  std::map<std::string, CampaignInfo> campaigns;

  for (const auto& creative_ad : creative_ads) {
    if (!exclusion_rule.ShouldInclude(creative_ad)) {
      continue;
    }

    ++active_ad_count;

    CampaignInfo& campaign = campaigns[creative_ad.campaign_id];
    campaign.advertiser_id = creative_ad.advertiser_id;
    campaign.start_at = creative_ad.start_at;
    campaign.end_at = creative_ad.end_at;
    campaign.daily_cap = creative_ad.daily_cap;
    campaign.priority = creative_ad.priority;
    campaign.pass_through_rate = creative_ad.pass_through_rate;
    campaign.metric_type = ToString(creative_ad.metric_type);
    campaign.geo_targets.insert(creative_ad.geo_targets.cbegin(),
                                creative_ad.geo_targets.cend());
    campaign.dayparts.insert(creative_ad.dayparts.cbegin(),
                             creative_ad.dayparts.cend());

    CreativeSetInfo& creative_set =
        campaign.creative_sets_by_id[creative_ad.creative_set_id];
    creative_set.per_day = creative_ad.per_day;
    creative_set.per_week = creative_ad.per_week;
    creative_set.per_month = creative_ad.per_month;
    creative_set.total_max = creative_ad.total_max;
    creative_set.segments.insert(creative_ad.segment);

    CreativeInfo& creative =
        creative_set.creatives_by_instance_id[creative_ad.creative_instance_id];
    creative.target_url = creative_ad.target_url.spec();
    creative.company_name = GetCompanyName(creative_ad);
    creative.alt = GetAlt(creative_ad);
    creative.title = GetTitle(creative_ad);
    creative.body = GetBody(creative_ad);
    creative.wallpaper_type = GetWallpaperType(creative_ad);
  }

  base::ListValue list;
  for (const auto& [campaign_id, campaign] : campaigns) {
    base::ListValue creative_sets;
    for (const auto& [creative_set_id, creative_set] :
         campaign.creative_sets_by_id) {
      base::ListValue creatives;
      for (const auto& [creative_instance_id, creative] :
           creative_set.creatives_by_instance_id) {
        base::DictValue creative_dict =
            base::DictValue()
                .Set("Creative Instance ID", creative_instance_id)
                .Set("Target URL", creative.target_url);
        if (!creative.company_name.empty()) {
          creative_dict.Set("Company Name", creative.company_name);
        }
        if (!creative.alt.empty()) {
          creative_dict.Set("Alt", creative.alt);
        }
        if (!creative.title.empty()) {
          creative_dict.Set("Title", creative.title);
        }
        if (!creative.body.empty()) {
          creative_dict.Set("Body", creative.body);
        }
        if (creative.wallpaper_type !=
            CreativeNewTabPageAdWallpaperType::kUndefined) {
          creative_dict.Set("Dynamic/Static",
                            ToString(creative.wallpaper_type));
        }
        creatives.Append(std::move(creative_dict));
      }
      base::ListValue segments;
      for (const auto& segment : creative_set.segments) {
        segments.Append(segment);
      }

      const std::vector<base::Time>* const creative_set_timestamps =
          base::FindOrNull(served_impression_timestamps.by_creative_set_id,
                           creative_set_id);

      creative_sets.Append(
          base::DictValue()
              .Set("Creative Set ID", creative_set_id)
              .Set("Segments", std::move(segments))
              .Set("Per Day", creative_set.per_day)
              .Set("Per Day Served",
                   CountServedImpressions(creative_set_timestamps,
                                          base::Days(1)))
              .Set("Per Week", creative_set.per_week)
              .Set("Per Week Served",
                   CountServedImpressions(creative_set_timestamps,
                                          base::Days(7)))
              .Set("Per Month", creative_set.per_month)
              .Set("Per Month Served",
                   CountServedImpressions(creative_set_timestamps,
                                          base::Days(28)))
              .Set("Total Max", creative_set.total_max)
              .Set("Total Max Served",
                   CountServedImpressions(creative_set_timestamps,
                                          /*time_constraint=*/std::nullopt))
              .Set("Creatives", std::move(creatives)));
    }

    base::ListValue geo_targets;
    for (const auto& geo_target : campaign.geo_targets) {
      geo_targets.Append(geo_target);
    }

    base::ListValue dayparts;
    for (const auto& daypart : campaign.dayparts) {
      dayparts.Append(base::DictValue()
                          .Set("Days Of Week", daypart.days_of_week)
                          .Set("Start Minute", daypart.start_minute)
                          .Set("End Minute", daypart.end_minute));
    }

    const std::vector<base::Time>* const campaign_timestamps = base::FindOrNull(
        served_impression_timestamps.by_campaign_id, campaign_id);

    list.Append(
        base::DictValue()
            .Set("Advertiser ID", campaign.advertiser_id)
            .Set("Campaign ID", campaign_id)
            .Set("Daily Cap", campaign.daily_cap)
            .Set("Daily Cap Served",
                 CountServedImpressions(campaign_timestamps, base::Days(1)))
            .Set("Priority", campaign.priority)
            .Set("Pass Through Rate", campaign.pass_through_rate)
            .Set("Metric Type", campaign.metric_type)
            .Set("Start At", campaign.start_at.InSecondsFSinceUnixEpoch())
            .Set("End At", campaign.end_at.InSecondsFSinceUnixEpoch())
            .Set("Geo Targets", std::move(geo_targets))
            .Set("Dayparts", std::move(dayparts))
            .Set("Creative Sets", std::move(creative_sets)));
  }

  std::move(callback).Run(/*success=*/true, active_ad_count, std::move(list));
}

// Frequency cap usage (see `CountServedImpressions`) needs the served
// impression history, fetched here so `BuildCampaigns` doesn't have to know
// where it came from.
template <typename T>
void GetAdEventsForCampaignsCallback(GetCampaignsDiagnosticsCallback callback,
                                     bool success,
                                     const SegmentList& segments,
                                     const std::vector<T>& creative_ads,
                                     bool ad_events_success,
                                     const AdEventList& ad_events) {
  if (!ad_events_success) {
    return std::move(callback).Run(/*success=*/false, /*active_ad_count=*/0,
                                   base::ListValue());
  }

  BuildCampaigns(std::move(callback), ad_events, success, segments,
                 creative_ads);
}

}  // namespace

void GetNotificationAdCampaignsCallback(
    GetCampaignsDiagnosticsCallback callback,
    bool success,
    const SegmentList& segments,
    CreativeNotificationAdList creative_ads) {
  if (!success) {
    return BuildCampaigns(std::move(callback), /*ad_events=*/{}, success,
                          segments, creative_ads);
  }

  database::table::AdEvents ad_events_database_table;
  ad_events_database_table.GetUnexpired(base::BindOnce(
      &GetAdEventsForCampaignsCallback<CreativeNotificationAdInfo>,
      std::move(callback), success, segments, std::move(creative_ads)));
}

void GetNewTabPageAdCampaignsCallback(GetCampaignsDiagnosticsCallback callback,
                                      bool success,
                                      const SegmentList& segments,
                                      CreativeNewTabPageAdList creative_ads) {
  if (!success) {
    return BuildCampaigns(std::move(callback), /*ad_events=*/{}, success,
                          segments, creative_ads);
  }

  database::table::AdEvents ad_events_database_table;
  ad_events_database_table.GetUnexpired(base::BindOnce(
      &GetAdEventsForCampaignsCallback<CreativeNewTabPageAdInfo>,
      std::move(callback), success, segments, std::move(creative_ads)));
}

}  // namespace brave_ads
