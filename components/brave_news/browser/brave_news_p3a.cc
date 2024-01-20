// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_p3a.h"

#include <algorithm>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_news {
namespace p3a {

namespace {

constexpr int kCardViewBuckets[] = {0, 1, 10, 20, 40, 80, 100};
constexpr int kCardVisitBuckets[] = {2, 5, 10, 15, 20, 25};
constexpr int kSidebarFilterUsageBuckets[] = {1, 4, 7, 10};
constexpr int kCardVisitDepthBuckets[] = {3, 6, 10, 15, 20};
constexpr int kSubscriptionCountBuckets[] = {1, 4, 7, 10};
constexpr size_t kCardVisitDepthMetricThreshold = 5;
constexpr base::TimeDelta kMonthlyUserTimeThreshold = base::Days(30);
constexpr base::TimeDelta kReportInterval = base::Days(1);

constexpr ActionType kAllActionTypes[] = {ActionType::kCardView,
                                          ActionType::kCardVisit,
                                          ActionType::kSidebarFilterUsage};
constexpr SubscribeType kAllSubscriptionTypes[] = {SubscribeType::kChannels,
                                                   SubscribeType::kPublishers};

void RecordGeneralUsage() {
  UMA_HISTOGRAM_BOOLEAN(kUsageMonthlyHistogramName, true);
  UMA_HISTOGRAM_BOOLEAN(kUsageDailyHistogramName, true);
}

}  // namespace

NewsMetrics::NewsMetrics(PrefService* prefs) : prefs_(prefs) {}

NewsMetrics::~NewsMetrics() = default;

void NewsMetrics::RecordAtSessionStart() {
  p3a_utils::RecordFeatureUsage(prefs_, prefs::kBraveNewsFirstSessionTime,
                                prefs::kBraveNewsLastSessionTime);

  RecordLastUsageTime();
  RecordNewUserReturning();
  RecordGeneralUsage();
  RecordDirectFeedsTotal();

  for (const auto subscribe_type : kAllSubscriptionTypes) {
    RecordTotalSubscribedCount(subscribe_type, {});
  }

  RecordWeeklySessionCount(true);
}

void NewsMetrics::RecordWeeklyDisplayAdsViewedCount(bool is_add) {
  // Store current weekly total in p3a, ready to send on the next upload
  constexpr int buckets[] = {0, 1, 4, 8, 14, 30, 60, 120};
  uint64_t total = AddToWeeklyStorageAndGetSum(
      prefs::kBraveNewsWeeklyDisplayAdViewedCount, is_add);
  p3a_utils::RecordToHistogramBucket(kWeeklyDisplayAdsViewedHistogramName,
                                     buckets, total);
}

void NewsMetrics::RecordDirectFeedsTotal() {
  if (!IsMonthlyActiveUser()) {
    // Only report for active users in the past month.
    return;
  }

  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  const auto& direct_feeds_dict = prefs_->GetDict(prefs::kBraveNewsDirectFeeds);
  std::size_t feed_count = direct_feeds_dict.size();
  p3a_utils::RecordToHistogramBucket(kDirectFeedsTotalHistogramName, buckets,
                                     feed_count);
}

void NewsMetrics::RecordWeeklyAddedDirectFeedsCount(int change) {
  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  uint64_t weekly_total = AddToWeeklyStorageAndGetSum(
      prefs::kBraveNewsWeeklyAddedDirectFeedsCount, change);

  p3a_utils::RecordToHistogramBucket(kWeeklyAddedDirectFeedsHistogramName,
                                     buckets, weekly_total);
}

void NewsMetrics::RecordTotalActionCount(ActionType action,
                                         uint64_t count_delta) {
  const char* pref_name;
  switch (action) {
    case ActionType::kCardView:
      pref_name = prefs::kBraveNewsTotalCardViews;
      break;
    case ActionType::kCardVisit:
      pref_name = prefs::kBraveNewsTotalCardVisits;
      break;
    case ActionType::kSidebarFilterUsage:
      pref_name = prefs::kBraveNewsTotalSidebarFilterUsages;
      break;
    default:
      NOTREACHED();
      return;
  }

  WeeklyStorage total_storage(prefs_, pref_name);

  total_storage.AddDelta(count_delta);

  uint64_t total = total_storage.GetWeeklySum();

  if (total == 0 && action != ActionType::kCardView) {
    // Only report 0 for the card views metric.
    return;
  }

  VLOG(1) << "NewsP3A: total actions update: total = " << total
          << " count delta = " << count_delta
          << " action enum = " << static_cast<int>(action);

  switch (action) {
    case ActionType::kCardView:
      p3a_utils::RecordToHistogramBucket(kTotalCardViewsHistogramName,
                                         kCardViewBuckets, total);
      break;
    case ActionType::kCardVisit:
      p3a_utils::RecordToHistogramBucket(kTotalCardVisitsHistogramName,
                                         kCardVisitBuckets, total);
      break;
    case ActionType::kSidebarFilterUsage:
      p3a_utils::RecordToHistogramBucket(kSidebarFilterUsageHistogramName,
                                         kSidebarFilterUsageBuckets, total);
      break;
    default:
      NOTREACHED();
  }
}

void NewsMetrics::RecordVisitCardDepth(uint32_t new_visit_card_depth) {
  WeeklyStorage total_visits_storage(prefs_, prefs::kBraveNewsTotalCardVisits);
  auto total_visits = total_visits_storage.GetWeeklySum();

  WeeklyStorage visit_depth_sum_storage(prefs_, prefs::kBraveNewsVisitDepthSum);

  VLOG(1) << "NewsP3A: card depth update: new_visit_card_depth = " << new_visit_card_depth;

  if (new_visit_card_depth > 0) {
    visit_depth_sum_storage.AddDelta(new_visit_card_depth);
  }

  if (total_visits < kCardVisitDepthMetricThreshold) {
    // Do not report if below defined threshold in the question.
    return;
  }

  auto depth_sum = visit_depth_sum_storage.GetWeeklySum();

  int average = static_cast<int>(static_cast<double>(depth_sum) / total_visits);

  p3a_utils::RecordToHistogramBucket(kVisitDepthHistogramName,
                                     kCardVisitDepthBuckets, average);
}

void NewsMetrics::RecordTotalSubscribedCount(SubscribeType subscribe_type,
                                             std::optional<size_t> total) {
  if (total) {
    subscription_counts_[subscribe_type] = *total;
  }

  const char* histogram_name;
  switch (subscribe_type) {
    case SubscribeType::kChannels:
      histogram_name = kChannelCountHistogramName;
      break;
    case SubscribeType::kPublishers:
      histogram_name = kPublisherCountHistogramName;
      break;
    default:
      NOTREACHED();
      return;
  }

  if (!IsMonthlyActiveUser()) {
    // Only report for active users in the past month.
    return;
  }

  if (subscription_counts_.contains(subscribe_type)) {
    p3a_utils::RecordToHistogramBucket(histogram_name,
                                       kSubscriptionCountBuckets,
                                       subscription_counts_[subscribe_type]);
  }
}

void NewsMetrics::RecordFeatureEnabledChange() {
  bool enabled = IsNewsEnabled();
  bool was_ever_enabled = prefs_->GetBoolean(prefs::kBraveNewsWasEverEnabled);
  if (!enabled && !was_ever_enabled) {
    // If the user clicked "no thanks" on the NTP, then we don't want to record
    // this as an opt-out, since they were never opted in.
    return;
  }
  prefs_->SetBoolean(prefs::kBraveNewsWasEverEnabled, true);
  UMA_HISTOGRAM_BOOLEAN(kIsEnabledHistogramName, enabled);
}

void NewsMetrics::RecordAtInit() {
  RecordLastUsageTime();
  RecordNewUserReturning();

  RecordDirectFeedsTotal();
  RecordWeeklyAddedDirectFeedsCount(0);
  RecordWeeklySessionCount(false);
  RecordWeeklyDisplayAdsViewedCount(false);

  for (const auto action : kAllActionTypes) {
    RecordTotalActionCount(action, 0);
  }
  RecordVisitCardDepth({});

  if (IsNewsEnabled()) {
    prefs_->SetBoolean(prefs::kBraveNewsWasEverEnabled, true);
  }

  report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&NewsMetrics::RecordAtInit, base::Unretained(this)));
}

bool NewsMetrics::IsNewsEnabled() {
  return prefs_->GetBoolean(prefs::kBraveNewsOptedIn) &&
         prefs_->GetBoolean(prefs::kNewTabPageShowToday);
}

bool NewsMetrics::IsMonthlyActiveUser() {
  base::Time last_usage = prefs_->GetTime(prefs::kBraveNewsLastSessionTime);
  return base::Time::Now() - last_usage < kMonthlyUserTimeThreshold;
}

uint64_t NewsMetrics::AddToWeeklyStorageAndGetSum(const char* pref_name,
                                                  int change) {
  WeeklyStorage storage(prefs_, pref_name);
  if (change > 0) {
    storage.AddDelta(1);
  } else if (change < 0) {
    storage.SubDelta(1);
  }
  return storage.GetWeeklySum();
}

void NewsMetrics::RecordLastUsageTime() {
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      prefs_, prefs::kBraveNewsLastSessionTime, kLastUsageTimeHistogramName);
}

void NewsMetrics::RecordNewUserReturning() {
  p3a_utils::RecordFeatureNewUserReturning(
      prefs_, prefs::kBraveNewsFirstSessionTime,
      prefs::kBraveNewsLastSessionTime, prefs::kBraveNewsUsedSecondDay,
      kNewUserReturningHistogramName);
}

void NewsMetrics::RecordWeeklySessionCount(bool is_add) {
  // Track how many times in the past week
  // user has scrolled to Brave News.
  constexpr int buckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  uint64_t total_session_count =
      AddToWeeklyStorageAndGetSum(prefs::kBraveNewsWeeklySessionCount, is_add);
  p3a_utils::RecordToHistogramBucket(kWeeklySessionCountHistogramName, buckets,
                                     total_session_count);
}

void NewsMetrics::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveNewsWeeklySessionCount);
  registry->RegisterListPref(prefs::kBraveNewsWeeklyDisplayAdViewedCount);
  registry->RegisterListPref(prefs::kBraveNewsWeeklyAddedDirectFeedsCount);
  registry->RegisterListPref(prefs::kBraveNewsTotalCardViews);
  registry->RegisterListPref(prefs::kBraveNewsTotalCardVisits);
  registry->RegisterListPref(prefs::kBraveNewsVisitDepthSum);
  registry->RegisterListPref(prefs::kBraveNewsTotalSidebarFilterUsages);
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, prefs::kBraveNewsFirstSessionTime,
      prefs::kBraveNewsLastSessionTime, prefs::kBraveNewsUsedSecondDay, nullptr,
      nullptr);
  registry->RegisterBooleanPref(prefs::kBraveNewsWasEverEnabled, false);
}

void NewsMetrics::RegisterProfilePrefsForMigration(
    PrefRegistrySimple* registry) {
  // Reserved for future deprecated P3A-related prefs
}

void NewsMetrics::MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Reserved for future deprecated P3A-related prefs
}

}  // namespace p3a
}  // namespace brave_news
