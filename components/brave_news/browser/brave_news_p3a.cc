// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_p3a.h"

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/common/p3a_pref_names.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_service.h"

namespace brave_news::p3a {

namespace {

constexpr int kCardViewBuckets[] = {0, 1, 10, 20, 40, 80, 100};
constexpr int kCardClickBuckets[] = {2, 5, 10, 15, 20, 25};
constexpr int kSidebarFilterUsageBuckets[] = {1, 4, 7, 10};
constexpr int kClickCardDepthBuckets[] = {3, 6, 10, 15, 20};
constexpr int kSubscriptionCountBuckets[] = {0, 1, 4, 8, 13};
constexpr size_t kCardClickDepthMetricThreshold = 5;
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

NewsMetrics::NewsMetrics(PrefService* prefs, BraveNewsPrefManager& pref_manager)
    : prefs_(prefs),
      pref_manager_(pref_manager),
      was_enabled_(pref_manager_->IsEnabled()),
      direct_feed_count_(
          pref_manager_->GetSubscriptions().direct_feeds().size()) {
  observation_.Observe(&*pref_manager_);
}

NewsMetrics::~NewsMetrics() = default;

void NewsMetrics::RecordAtSessionStart() {
  DVLOG(1) << __FUNCTION__;
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
  DVLOG(1) << __FUNCTION__ << " is_add: " << is_add;
  // Store current weekly total in p3a, ready to send on the next upload
  constexpr int kBuckets[] = {0, 1, 4, 8, 14, 30, 60, 120};
  uint64_t total = AddToWeeklyStorageAndGetSum(
      prefs::kBraveNewsWeeklyDisplayAdViewedCount, is_add);
  const char* histogram_name = kNonRewardsAdsViewsHistogramName;
  const char* disabled_histogram_name = kRewardsAdsViewsHistogramName;
  if (prefs_->GetBoolean(brave_rewards::prefs::kEnabled)) {
    histogram_name = kRewardsAdsViewsHistogramName;
    disabled_histogram_name = kNonRewardsAdsViewsHistogramName;
  }
  p3a_utils::RecordToHistogramBucket(histogram_name, kBuckets, total);
  base::UmaHistogramExactLinear(disabled_histogram_name, INT_MAX - 1, 8);
}

void NewsMetrics::RecordDirectFeedsTotal() {
  DVLOG(1) << __FUNCTION__;
  if (!IsMonthlyActiveUser()) {
    VLOG(1) << "Not recording direct feed total (not monthly active user)";
    // Only report for active users in the past month.
    return;
  }

  std::size_t feed_count =
      pref_manager_->GetSubscriptions().direct_feeds().size();
  DVLOG(1) << "DirectFeedTotal: " << feed_count;
  p3a_utils::RecordToHistogramBucket(kDirectFeedsTotalHistogramName,
                                     kSubscriptionCountBuckets, feed_count);
}

void NewsMetrics::RecordWeeklyAddedDirectFeedsCount(int change) {
  DVLOG(1) << __FUNCTION__ << " change: " << change;
  constexpr int kBuckets[] = {0, 1, 2, 3, 4, 5, 10};
  uint64_t weekly_total = AddToWeeklyStorageAndGetSum(
      prefs::kBraveNewsWeeklyAddedDirectFeedsCount, change);

  p3a_utils::RecordToHistogramBucket(kWeeklyAddedDirectFeedsHistogramName,
                                     kBuckets, weekly_total);
}

void NewsMetrics::RecordTotalActionCount(ActionType action,
                                         uint64_t count_delta) {
  DVLOG(1) << __FUNCTION__ << " action: " << static_cast<int>(action)
           << ", count_delta: " << count_delta;
  const char* pref_name = nullptr;
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
  }
  CHECK(pref_name);

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
      p3a_utils::RecordToHistogramBucket(kTotalCardClicksHistogramName,
                                         kCardClickBuckets, total);
      break;
    case ActionType::kSidebarFilterUsage:
      p3a_utils::RecordToHistogramBucket(kSidebarFilterUsageHistogramName,
                                         kSidebarFilterUsageBuckets, total);
      break;
  }
}

void NewsMetrics::RecordVisitCardDepth(uint32_t new_visit_card_depth) {
  WeeklyStorage total_visits_storage(prefs_, prefs::kBraveNewsTotalCardVisits);
  auto total_visits = total_visits_storage.GetWeeklySum();

  WeeklyStorage visit_depth_sum_storage(prefs_, prefs::kBraveNewsVisitDepthSum);

  VLOG(1) << "NewsP3A: card depth update: new_visit_card_depth = "
          << new_visit_card_depth;

  if (new_visit_card_depth > 0) {
    visit_depth_sum_storage.AddDelta(new_visit_card_depth);
  }

  if (total_visits < kCardClickDepthMetricThreshold) {
    // Do not report if below defined threshold in the question.
    return;
  }

  auto depth_sum = visit_depth_sum_storage.GetWeeklySum();

  int average = static_cast<int>(static_cast<double>(depth_sum) / total_visits);

  p3a_utils::RecordToHistogramBucket(kClickCardDepthHistogramName,
                                     kClickCardDepthBuckets, average);
}

void NewsMetrics::RecordTotalSubscribedCount(SubscribeType subscribe_type,
                                             std::optional<size_t> total) {
  DVLOG(1) << __FUNCTION__
           << " subscribe_type: " << static_cast<int>(subscribe_type)
           << ", total: " << total.value_or(0);
  if (total) {
    subscription_counts_[subscribe_type] = *total;
  }

  const char* histogram_name = nullptr;
  switch (subscribe_type) {
    case SubscribeType::kChannels:
      histogram_name = kChannelCountHistogramName;
      break;
    case SubscribeType::kPublishers:
      histogram_name = kPublisherCountHistogramName;
      break;
  }
  CHECK(histogram_name);

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
  bool enabled = pref_manager_->IsEnabled();
  bool was_ever_enabled = prefs_->GetBoolean(prefs::kBraveNewsWasEverEnabled);
  if (!enabled && !was_ever_enabled) {
    // If the user clicked "no thanks" on the NTP, then we don't want to record
    // this as an opt-out, since they were never opted in.
    return;
  }
  DVLOG(1) << __FUNCTION__ << " is_enabled: " << enabled;

  prefs_->SetBoolean(prefs::kBraveNewsWasEverEnabled, true);
  UMA_HISTOGRAM_BOOLEAN(kIsEnabledHistogramName, enabled);
}

void NewsMetrics::RecordAtInit() {
  DVLOG(1) << __FUNCTION__;

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

  if (pref_manager_->IsEnabled()) {
    prefs_->SetBoolean(prefs::kBraveNewsWasEverEnabled, true);
  }

  report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&NewsMetrics::RecordAtInit, base::Unretained(this)));
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
  DVLOG(1) << __FUNCTION__;
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      prefs_, prefs::kBraveNewsLastSessionTime, kLastUsageTimeHistogramName);
}

void NewsMetrics::RecordNewUserReturning() {
  DVLOG(1) << __FUNCTION__;
  p3a_utils::RecordFeatureNewUserReturning(
      prefs_, prefs::kBraveNewsFirstSessionTime,
      prefs::kBraveNewsLastSessionTime, prefs::kBraveNewsUsedSecondDay,
      kNewUserReturningHistogramName);
}

void NewsMetrics::RecordWeeklySessionCount(bool is_add) {
  DVLOG(1) << __FUNCTION__ << " is_add: " << is_add;
  // Track how many times in the past week
  // user has scrolled to Brave News.
  constexpr int kBuckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  uint64_t total_session_count =
      AddToWeeklyStorageAndGetSum(prefs::kBraveNewsWeeklySessionCount, is_add);
  p3a_utils::RecordToHistogramBucket(kWeeklySessionCountHistogramName, kBuckets,
                                     total_session_count);
}

void NewsMetrics::OnConfigChanged() {
  DVLOG(1) << __FUNCTION__;
  if (was_enabled_ == pref_manager_->IsEnabled()) {
    return;
  }
  was_enabled_ = pref_manager_->IsEnabled();
  RecordFeatureEnabledChange();

  // Get initial publisher count, which should be 0.
  OnPublishersChanged();
}

void NewsMetrics::OnPublishersChanged() {
  DVLOG(1) << __FUNCTION__;
  auto subscriptions = pref_manager_->GetSubscriptions();
  RecordTotalSubscribedCount(SubscribeType::kPublishers,
                             subscriptions.enabled_publishers().size());

  // If the number of direct publishers has changed, record the total and the
  // delta.
  auto direct_publishers_delta =
      subscriptions.direct_feeds().size() - direct_feed_count_;
  if (direct_publishers_delta != 0) {
    RecordDirectFeedsTotal();
    RecordWeeklyAddedDirectFeedsCount(direct_publishers_delta);
    direct_feed_count_ += direct_publishers_delta;
  }
}

void NewsMetrics::OnChannelsChanged() {
  DVLOG(1) << __FUNCTION__;
  base::flat_set<std::string> distinct_channels;
  for (const auto& [locale, channels] :
       pref_manager_->GetSubscriptions().channels()) {
    distinct_channels.insert(channels.begin(), channels.end());
  }
  RecordTotalSubscribedCount(SubscribeType::kChannels,
                             distinct_channels.size());
}

}  // namespace brave_news::p3a
