/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_internals/ads_internals_util.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/barrier_closure.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/ads_internals/ads_internals_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/history/ad_history_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

// A creative set conversion isn't itself tied to a specific ad format, unlike
// an ad event; there's no local database table for search result ads to look
// up, so that format is inferred by elimination: present in neither set.
std::string_view GetAdType(
    const std::string& creative_set_id,
    const base::flat_set<std::string>& notification_ad_creative_set_ids,
    const base::flat_set<std::string>& new_tab_page_ad_creative_set_ids) {
  if (notification_ad_creative_set_ids.contains(creative_set_id)) {
    return ToString(mojom::AdType::kNotificationAd);
  }
  if (new_tab_page_ad_creative_set_ids.contains(creative_set_id)) {
    return ToString(mojom::AdType::kNewTabPageAd);
  }
  return ToString(mojom::AdType::kSearchResultAd);
}

base::ListValue BuildCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions,
    const base::flat_set<std::string>& notification_ad_creative_set_ids,
    const base::flat_set<std::string>& new_tab_page_ad_creative_set_ids) {
  base::ListValue list;
  list.reserve(creative_set_conversions.size());

  for (const auto& creative_set_conversion : creative_set_conversions) {
    if (!creative_set_conversion.IsValid()) {
      // Skip invalid creative set conversions.
      continue;
    }

    list.Append(
        base::DictValue()
            .Set("Creative Set ID", creative_set_conversion.id)
            .Set("Ad Type", GetAdType(creative_set_conversion.id,
                                      notification_ad_creative_set_ids,
                                      new_tab_page_ad_creative_set_ids))
            .Set("URL Pattern", creative_set_conversion.url_pattern)
            .Set("Expires At",
                 creative_set_conversion.expire_at->InSecondsFSinceUnixEpoch())
            .Set("Observation Window",
                 static_cast<int>(
                     creative_set_conversion.observation_window.InDays())));
  }

  return list;
}

base::ListValue BuildAdEvents(const AdEventList& ad_events) {
  base::ListValue list;
  list.reserve(ad_events.size());

  for (const auto& ad_event : ad_events) {
    if (!ad_event.IsValid()) {
      // Skip invalid ad events.
      continue;
    }

    if (ad_event.confirmation_type ==
        mojom::ConfirmationType::kServedImpression) {
      // Skip served impressions.
      continue;
    }

    base::DictValue dict =
        base::DictValue()
            .Set("Placement ID", ad_event.placement_id)
            .Set("Creative Instance ID", ad_event.creative_instance_id)
            .Set("Target URL", ad_event.target_url.spec())
            .Set("Ad Type", ToString(ad_event.type))
            .Set("Event Type", ToString(ad_event.confirmation_type))
            .Set("Created At", ad_event.created_at->InSecondsFSinceUnixEpoch());
    if (ad_event.segment != kUntargetedSegment) {
      dict.Set("Segment", ad_event.segment);
    }
    list.Append(std::move(dict));
  }

  return list;
}

base::ListValue BuildDislikedIds(const ReactionMap& reactions) {
  base::ListValue list;

  for (const auto& [id, type] : reactions) {
    if (type == mojom::ReactionType::kDisliked) {
      list.Append(id);
    }
  }

  return list;
}

base::ListValue BuildLikedIds(const ReactionMap& reactions) {
  base::ListValue list;

  for (const auto& [id, type] : reactions) {
    if (type == mojom::ReactionType::kLiked) {
      list.Append(id);
    }
  }

  return list;
}

base::ListValue BuildIds(const ReactionSet& ids) {
  base::ListValue list;
  list.reserve(ids.size());

  for (const auto& id : ids) {
    list.Append(id);
  }

  return list;
}

void Failed(GetInternalsCallback callback) {
  BLOG(0, "Failed to get ads internals");
  std::move(callback).Run(/*internals=*/std::nullopt);
}

void Successful(GetInternalsCallback callback, AdsInternalsInfo ads_internals) {
  const Reactions& reactions = GetReactions();

  base::DictValue dict =
      base::DictValue()
          .Set("creativeSetConversions",
               BuildCreativeSetConversions(
                   ads_internals.creative_set_conversions,
                   ads_internals.notification_ad_creative_set_ids,
                   ads_internals.new_tab_page_ad_creative_set_ids))
          .Set("adEvents", BuildAdEvents(ads_internals.ad_events))
          .Set("confirmationQueue", std::move(ads_internals.confirmation_queue))
          .Set("paymentTokens", std::move(ads_internals.payment_tokens))
          .Set("activeNotificationAdCount",
               static_cast<int>(ads_internals.active_notification_ad_count))
          .Set("activeNewTabPageAdCount",
               static_cast<int>(ads_internals.active_new_tab_page_ad_count))
          .Set("activeNotificationAdCampaigns",
               std::move(ads_internals.active_notification_ad_campaigns))
          .Set("activeNewTabPageAdCampaigns",
               std::move(ads_internals.active_new_tab_page_ad_campaigns))
          .Set("conditionMatchers", std::move(ads_internals.condition_matchers))
          .Set("transactions", std::move(ads_internals.transactions))
          .Set("dislikedAds", BuildDislikedIds(reactions.Ads()))
          .Set("dislikedSegments", BuildDislikedIds(reactions.Segments()))
          .Set("likedAds", BuildLikedIds(reactions.Ads()))
          .Set("likedSegments", BuildLikedIds(reactions.Segments()))
          .Set("savedAds", BuildIds(reactions.SavedAds()))
          .Set("adsMarkedAsInappropriate",
               BuildIds(reactions.MarkedAdsAsInappropriate()))
          .Set("adHistoryRetentionPeriodDays",
               static_cast<int>(kAdHistoryRetentionPeriod.Get().InDays()));

  const base::Time next_payment_token_redemption_at =
      GetProfileTimePref(prefs::kNextPaymentTokenRedemptionAt);
  if (!next_payment_token_redemption_at.is_null()) {
    dict.Set("nextPaymentTokenRedemptionAt",
             next_payment_token_redemption_at.InSecondsFSinceUnixEpoch());
  }

  // New tab page ads are excluded until the grace period ends (see
  // `GracePeriodExclusionRule`).
  const base::Time grace_period_end_at =
      GetLocalStateTimePref(prefs::kFirstRunAt) +
      GetProfileTimeDeltaPref(prefs::kGracePeriod);
  dict.Set("newTabPageAdGracePeriodEndAt",
           grace_period_end_at.InSecondsFSinceUnixEpoch());

  std::move(callback).Run(std::move(dict));
}

// Bundles the six independent `DiagnosticManager` queries below (none of
// them depends on another's result) plus whether all of them succeeded, so
// they can fan out concurrently and fan back in through one
// `base::BarrierClosure` instead of daisy-chaining six sequential round
// trips to the database.
struct AdsInternalsBarrierResult {
  AdsInternalsInfo ads_internals;
  bool success = true;
};

void OnAllInternalsCollected(
    GetInternalsCallback callback,
    std::unique_ptr<AdsInternalsBarrierResult> barrier_result) {
  if (!barrier_result->success) {
    return Failed(std::move(callback));
  }

  Successful(std::move(callback), std::move(barrier_result->ads_internals));
}

void GetTransactionsForInternalsCallback(
    AdsInternalsBarrierResult* barrier_result,
    base::RepeatingClosure barrier_closure,
    bool success,
    base::ListValue transactions) {
  if (success) {
    barrier_result->ads_internals.transactions = std::move(transactions);
  } else {
    barrier_result->success = false;
  }
  barrier_closure.Run();
}

void GetConditionMatchersCallback(AdsInternalsBarrierResult* barrier_result,
                                  base::RepeatingClosure barrier_closure,
                                  bool success,
                                  base::ListValue condition_matchers) {
  if (success) {
    barrier_result->ads_internals.condition_matchers =
        std::move(condition_matchers);
  } else {
    barrier_result->success = false;
  }
  barrier_closure.Run();
}

void GetNewTabPageAdCampaignsCallback(AdsInternalsBarrierResult* barrier_result,
                                      base::RepeatingClosure barrier_closure,
                                      bool success,
                                      size_t active_new_tab_page_ad_count,
                                      base::ListValue campaigns) {
  if (success) {
    barrier_result->ads_internals.active_new_tab_page_ad_count =
        active_new_tab_page_ad_count;
    barrier_result->ads_internals.active_new_tab_page_ad_campaigns =
        std::move(campaigns);
  } else {
    barrier_result->success = false;
  }
  barrier_closure.Run();
}

void GetNotificationAdCampaignsCallback(
    AdsInternalsBarrierResult* barrier_result,
    base::RepeatingClosure barrier_closure,
    bool success,
    size_t active_notification_ad_count,
    base::ListValue campaigns) {
  if (success) {
    barrier_result->ads_internals.active_notification_ad_count =
        active_notification_ad_count;
    barrier_result->ads_internals.active_notification_ad_campaigns =
        std::move(campaigns);
  } else {
    barrier_result->success = false;
  }
  barrier_closure.Run();
}

void GetPaymentTokensForInternalsCallback(
    AdsInternalsBarrierResult* barrier_result,
    base::RepeatingClosure barrier_closure,
    bool success,
    base::ListValue payment_tokens) {
  if (success) {
    barrier_result->ads_internals.payment_tokens = std::move(payment_tokens);
  } else {
    barrier_result->success = false;
  }
  barrier_closure.Run();
}

void GetConfirmationQueueForInternalsCallback(
    AdsInternalsBarrierResult* barrier_result,
    base::RepeatingClosure barrier_closure,
    bool success,
    base::ListValue confirmation_queue) {
  if (success) {
    barrier_result->ads_internals.confirmation_queue =
        std::move(confirmation_queue);
  } else {
    barrier_result->success = false;
  }
  barrier_closure.Run();
}

void GetUnexpiredAdEventsCallback(GetInternalsCallback callback,
                                  AdsInternalsInfo ads_internals,
                                  bool success,
                                  const AdEventList& ad_events) {
  if (!success) {
    return Failed(std::move(callback));
  }

  ads_internals.ad_events = ad_events;

  auto barrier_result = std::make_unique<AdsInternalsBarrierResult>();
  barrier_result->ads_internals = std::move(ads_internals);
  AdsInternalsBarrierResult* const barrier_result_ptr = barrier_result.get();

  static constexpr size_t kInternalsQueryCount = 6;
  base::RepeatingClosure barrier_closure = base::BarrierClosure(
      kInternalsQueryCount,
      base::BindOnce(&OnAllInternalsCollected, std::move(callback),
                     std::move(barrier_result)));

  DiagnosticManager::GetConfirmationQueue(
      base::BindOnce(&GetConfirmationQueueForInternalsCallback,
                     barrier_result_ptr, barrier_closure));
  DiagnosticManager::GetPaymentTokens(
      base::BindOnce(&GetPaymentTokensForInternalsCallback, barrier_result_ptr,
                     barrier_closure));
  DiagnosticManager::GetNotificationAdCampaigns(
      base::BindOnce(&GetNotificationAdCampaignsCallback, barrier_result_ptr,
                     barrier_closure));
  DiagnosticManager::GetNewTabPageAdCampaigns(base::BindOnce(
      &GetNewTabPageAdCampaignsCallback, barrier_result_ptr, barrier_closure));
  DiagnosticManager::GetConditionMatchers(base::BindOnce(
      &GetConditionMatchersCallback, barrier_result_ptr, barrier_closure));
  DiagnosticManager::GetTransactions(
      base::BindOnce(&GetTransactionsForInternalsCallback, barrier_result_ptr,
                     barrier_closure));
}

void GetNewTabPageAdCreativeSetIdsCallback(
    GetInternalsCallback callback,
    AdsInternalsInfo ads_internals,
    bool success,
    const SegmentList& /*segments*/,
    CreativeNewTabPageAdList creative_ads) {
  if (!success) {
    return Failed(std::move(callback));
  }

  for (const auto& creative_ad : creative_ads) {
    ads_internals.new_tab_page_ad_creative_set_ids.insert(
        creative_ad.creative_set_id);
  }

  database::table::AdEvents database_table;
  database_table.GetUnexpired(base::BindOnce(&GetUnexpiredAdEventsCallback,
                                             std::move(callback),
                                             std::move(ads_internals)));
}

void GetNotificationAdCreativeSetIdsCallback(
    GetInternalsCallback callback,
    AdsInternalsInfo ads_internals,
    bool success,
    const SegmentList& /*segments*/,
    CreativeNotificationAdList creative_ads) {
  if (!success) {
    return Failed(std::move(callback));
  }

  for (const auto& creative_ad : creative_ads) {
    ads_internals.notification_ad_creative_set_ids.insert(
        creative_ad.creative_set_id);
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetAll(base::BindOnce(&GetNewTabPageAdCreativeSetIdsCallback,
                                       std::move(callback),
                                       std::move(ads_internals)));
}

void GetCreativeSetConversionsCallback(
    GetInternalsCallback callback,
    bool success,
    const CreativeSetConversionList& creative_set_conversions) {
  if (!success) {
    return Failed(std::move(callback));
  }

  AdsInternalsInfo ads_internals;
  ads_internals.creative_set_conversions = creative_set_conversions;

  database::table::CreativeNotificationAds database_table;
  database_table.GetAll(base::BindOnce(&GetNotificationAdCreativeSetIdsCallback,
                                       std::move(callback),
                                       std::move(ads_internals)));
}

}  // namespace

void BuildAdsInternals(GetInternalsCallback callback) {
  database::table::CreativeSetConversions database_table;
  database_table.GetActive(
      base::BindOnce(&GetCreativeSetConversionsCallback, std::move(callback)));
}

}  // namespace brave_ads
