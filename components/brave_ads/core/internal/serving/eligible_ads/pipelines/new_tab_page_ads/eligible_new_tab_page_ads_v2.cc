/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_id_helper.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
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
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

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
    const UserModelInfo& user_model,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback,
    bool success,
    const SegmentList& /*segments*/,
    const CreativeNewTabPageAdList& creative_ads) {
  if (!success) {
    BLOG(0, "Failed to get ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  FilterAndMaybePredictCreativeAd(user_model, creative_ads, ad_events,
                                  site_history, std::move(callback));
}

void EligibleNewTabPageAdsV2::FilterAndMaybePredictCreativeAd(
    const UserModelInfo& user_model,
    const CreativeNewTabPageAdList& creative_ads,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history,
    EligibleAdsCallback<CreativeNewTabPageAdList> callback) {
  TRACE_EVENT(kTraceEventCategory,
              "EligibleNewTabPageAds::FilterAndMaybePredictCreativeAd",
              "creative_ads", creative_ads.size(), "ad_events",
              ad_events.size(), "site_history", site_history.size());

  if (creative_ads.empty()) {
    BLOG(1, "No eligible ads");
    return std::move(callback).Run(/*eligible_ads=*/{});
  }

  CreativeNewTabPageAdList eligible_creative_ads = creative_ads;
  FilterIneligibleCreativeAds(eligible_creative_ads, ad_events, site_history);

  const PrioritizedCreativeAdBuckets<CreativeNewTabPageAdList> buckets =
      SortCreativeAdsIntoBucketsByPriority(eligible_creative_ads);

  LogNumberOfCreativeAdsPerBucket(buckets);

  // For each bucket of prioritized ads attempt to predict the most suitable ad
  // for the user in priority order.
  for (const auto& [priority, prioritized_eligible_creative_ads] : buckets) {
    const std::optional<CreativeNewTabPageAdInfo> predicted_creative_ad =
        MaybePredictCreativeAd(prioritized_eligible_creative_ads, user_model,
                               ad_events);
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
  BLOG(1, "No eligible ads out of " << creative_ads.size() << " ads");
  std::move(callback).Run(/*eligible_ads=*/{});
}

void EligibleNewTabPageAdsV2::ApplyConditionMatcher(
    CreativeNewTabPageAdList& creative_ads) {
  TRACE_EVENT(kTraceEventCategory, "ApplyConditionMatcher", "creative_ads",
              creative_ads.size());

  std::erase_if(creative_ads, [](const CreativeNewTabPageAdInfo& creative_ad) {
    const bool does_match_conditions =
        MatchConditions(creative_ad.condition_matchers);
    if (!does_match_conditions) {
      BLOG(1, "creativeInstanceId " << creative_ad.creative_instance_id
                                    << " does not match conditions");
    }

    return !does_match_conditions;
  });
}

void EligibleNewTabPageAdsV2::FilterIneligibleCreativeAds(
    CreativeNewTabPageAdList& creative_ads,
    const AdEventList& ad_events,
    const SiteHistoryList& site_history) {
  if (creative_ads.empty()) {
    return;
  }

  ApplyConditionMatcher(creative_ads);

  if (UserHasJoinedBraveRewards()) {
    NewTabPageAdExclusionRules exclusion_rules(
        ad_events, *subdivision_targeting_, *anti_targeting_resource_,
        site_history);

    ApplyExclusionRules(creative_ads, last_served_ad_, &exclusion_rules);
  }

  PaceCreativeAds(creative_ads);
}

}  // namespace brave_ads
