/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/debug/crash_logging.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_id_helper.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/new_tab_page_ads/new_tab_page_ad_exclusion_rules.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/priority/priority.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

namespace {

constexpr std::string_view kAdEventVirtualPrefQueryName = "ad_events";

base::flat_set<std::string> GetAdEventVirtualPrefQueryIds(
    const CreativeNewTabPageAdList& creative_ads) {
  base::flat_set<std::string> ad_event_virtual_pref_query_ids;

  for (const auto& creative_ad : creative_ads) {
    for (const auto& [pref_path, condition] : creative_ad.condition_matchers) {
      std::optional<std::string_view> pref_path_without_prefix =
          base::RemovePrefix(pref_path, "[virtual]:");
      if (!pref_path_without_prefix) {
        // Not a virtual pref path.
        continue;
      }

      const std::vector<std::string> components =
          base::SplitString(*pref_path_without_prefix, "|",
                            base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
      if (components.empty()) {
        // Invalid virtual pref query path.
        continue;
      }

      if (components[0] == kAdEventVirtualPrefQueryName) {
        if (components.size() == 3) {
          // Only handle virtual pref query paths with exactly three components,
          // i.e., `ad_events|<id>|<confirmation_type>`.
          ad_event_virtual_pref_query_ids.insert(/*id*/ components[1]);
        }
      }
    }
  }

  return ad_event_virtual_pref_query_ids;
}

}  // namespace

EligibleNewTabPageAdsV2::EligibleNewTabPageAdsV2(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : EligibleNewTabPageAdsBase(subdivision_targeting,
                                anti_targeting_resource) {}

EligibleNewTabPageAdsV2::~EligibleNewTabPageAdsV2() = default;

void EligibleNewTabPageAdsV2::GetForUserModel(
    UserModelInfo user_model,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  BLOG(1, "Get eligible new tab page ads");

  ad_events_database_table_.GetUnexpired(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&EligibleNewTabPageAdsV2::GetForUserModelCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void EligibleNewTabPageAdsV2::GetForUserModelCallback(
    UserModelInfo user_model,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(0, "Failed to get ad events");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  GetSiteHistory(std::move(user_model), ad_events, std::move(callback));
}

void EligibleNewTabPageAdsV2::GetSiteHistory(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      kTraceEventCategory, "EligibleNewTabPageAds::GetSiteHistory",
      TRACE_ID_WITH_SCOPE("EligibleNewTabPageAds", trace_id));

  GetAdsClient().GetSiteHistory(
      kSiteHistoryMaxCount.Get(), kSiteHistoryRecentDayRange.Get(),
      base::BindOnce(&EligibleNewTabPageAdsV2::GetSiteHistoryCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, std::move(callback), trace_id));
}

void EligibleNewTabPageAdsV2::GetSiteHistoryCallback(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    uint64_t trace_id,
    const SiteHistoryList& site_history) {
  TRACE_EVENT_NESTABLE_ASYNC_END1(
      kTraceEventCategory, "EligibleNewTabPageAds::GetSiteHistory",
      TRACE_ID_WITH_SCOPE("EligibleNewTabPageAds", trace_id), "site_history",
      site_history.size());

  GetEligibleAds(std::move(user_model), ad_events, site_history,
                 std::move(callback));
}

void EligibleNewTabPageAdsV2::GetEligibleAds(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  creative_ads_database_table_.GetForActiveCampaigns(
      base::BindOnce(&EligibleNewTabPageAdsV2::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), std::move(user_model),
                     ad_events, site_history, std::move(callback)));
}

void EligibleNewTabPageAdsV2::GetEligibleAdsCallback(
    UserModelInfo user_model,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    bool success,
    const SegmentList& /*segments*/,
    CreativeNewTabPageAdList creative_ads) {
  if (!success) {
    BLOG(0, "Failed to get ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  for (const auto& creative_ad : creative_ads) {
    if (base::Time::Now() < creative_ad.start_at ||
        base::Time::Now() > creative_ad.end_at) {
      SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                                creative_ad.creative_instance_id);
      SCOPED_CRASH_KEY_NUMBER("Issue50267", "wallpaper_type",
                              static_cast<int>(creative_ad.wallpaper_type));
      SCOPED_CRASH_KEY_NUMBER("Issue50267", "metric_type",
                              static_cast<int>(creative_ad.metric_type));
      SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                                "Campaign is not active");
      SCOPED_CRASH_KEY_STRING64(
          "Issue50267", "start_at",
          TimeToPrivacyPreservingIso8601(creative_ad.start_at));
      SCOPED_CRASH_KEY_STRING64(
          "Issue50267", "end_at",
          TimeToPrivacyPreservingIso8601(creative_ad.end_at));
      SCOPED_CRASH_KEY_STRING64(
          "Issue50267", "now",
          TimeToPrivacyPreservingIso8601(base::Time::Now()));
      DUMP_WILL_BE_NOTREACHED();
    }
  }

  FilterAndMaybePredictCreativeAd(std::move(user_model),
                                  std::move(creative_ads), ad_events,
                                  site_history, std::move(callback));
}

void EligibleNewTabPageAdsV2::ApplyConditionMatcher(
    CreativeNewTabPageAdList creative_ads,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const base::flat_set<std::string> ad_event_virtual_pref_query_ids =
      GetAdEventVirtualPrefQueryIds(creative_ads);

  ad_events_database_table_.GetVirtualPrefs(
      ad_event_virtual_pref_query_ids,
      base::BindOnce(&EligibleNewTabPageAdsV2::ApplyConditionMatcherCallback,
                     weak_factory_.GetWeakPtr(), std::move(creative_ads),
                     std::move(callback)));
}

void EligibleNewTabPageAdsV2::ApplyConditionMatcherCallback(
    CreativeNewTabPageAdList creative_ads,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    base::Value::Dict virtual_prefs) {
  virtual_prefs.Merge(GetAdsClient().GetVirtualPrefs());

  std::erase_if(creative_ads, [&virtual_prefs](
                                  const CreativeNewTabPageAdInfo& creative_ad) {
    const bool does_match_conditions =
        MatchConditions(virtual_prefs, creative_ad.condition_matchers);
    if (!does_match_conditions) {
      BLOG(1, "creativeInstanceId " << creative_ad.creative_instance_id
                                    << " does not match conditions");
    }

    return !does_match_conditions;
  });

  std::move(callback).Run(std::move(creative_ads));
}

void EligibleNewTabPageAdsV2::FilterAndMaybePredictCreativeAd(
    UserModelInfo user_model,
    CreativeNewTabPageAdList creative_ads,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      kTraceEventCategory,
      "EligibleNewTabPageAds::FilterAndMaybePredictCreativeAd",
      TRACE_ID_WITH_SCOPE("EligibleNewTabPageAds", trace_id));

  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  ApplyConditionMatcher(
      std::move(creative_ads),
      base::BindOnce(
          &EligibleNewTabPageAdsV2::FilterAndMaybePredictCreativeAdCallback,
          weak_factory_.GetWeakPtr(), std::move(user_model), ad_events,
          site_history, std::move(callback), trace_id));
}

void EligibleNewTabPageAdsV2::FilterAndMaybePredictCreativeAdCallback(
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    uint64_t trace_id,
    CreativeNewTabPageAdList creative_ads) {
  const size_t creative_ad_count = creative_ads.size();

  TRACE_EVENT_NESTABLE_ASYNC_END2(
      kTraceEventCategory,
      "EligibleNewTabPageAds::FilterAndMaybePredictCreativeAd",
      TRACE_ID_WITH_SCOPE("EligibleNewTabPageAds", trace_id), "ad_events",
      ad_events.size(), "creative_ads", creative_ad_count);

  if (kShouldFrequencyCapNewTabPageAdsForNonRewards.Get() ||
      UserHasJoinedBraveRewards()) {
    NewTabPageAdExclusionRules exclusion_rules(
        ad_events, *subdivision_targeting_, *anti_targeting_resource_,
        site_history);
    ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);
  }

  PaceCreativeAds(creative_ads);

  const PrioritizedCreativeAdBuckets<CreativeNewTabPageAdList> buckets =
      SortCreativeAdsIntoBucketsByPriority(creative_ads);

  LogNumberOfCreativeAdsPerBucket(buckets);

  // For each bucket of prioritized ads attempt to predict the most suitable ad
  // for the user in priority order.
  for (const auto& [priority, prioritized_creative_ads] : buckets) {
    std::optional<CreativeNewTabPageAdInfo> predicted_creative_ad =
        MaybePredictCreativeAd(prioritized_creative_ads, user_model, ad_events);
    if (!predicted_creative_ad) {
      // Could not predict an ad for this bucket, so continue to the next
      // bucket.
      continue;
    }

    BLOG(1, "Predicted ad with creative instance id "
                << predicted_creative_ad->creative_instance_id
                << " and a priority of " << priority);

    return std::move(callback).Run({*predicted_creative_ad});
  }

  // Could not predict an ad for any of the buckets.
  BLOG(1, "No eligible ads out of " << creative_ad_count << " ads");
  std::move(callback).Run(/*eligible_ads=*/{});
}

}  // namespace brave_ads
