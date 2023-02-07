// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_p3a.h"

#include <algorithm>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_news {
namespace p3a {

namespace {

constexpr uint64_t kMaxViewRate = 5;
constexpr base::TimeDelta kViewReportInterval = base::Seconds(1);

uint16_t UpdateWeeklyStorageWithValueAndGetMax(PrefService* prefs,
                                               const char* pref_name,
                                               const uint64_t total) {
  WeeklyStorage storage(prefs, pref_name);
  storage.ReplaceTodaysValueIfGreater(total);
  return storage.GetHighestValueInWeek();
}

uint64_t AddToWeeklyStorageAndGetSum(PrefService* prefs,
                                     const char* pref_name,
                                     int change) {
  WeeklyStorage storage(prefs, pref_name);
  if (change > 0)
    storage.AddDelta(1);
  else if (change < 0)
    storage.SubDelta(1);
  return storage.GetWeeklySum();
}

void RecordLastUsageTime(PrefService* prefs) {
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      prefs, prefs::kBraveTodayLastSessionTime, kLastUsageTimeHistogramName);
}

void RecordNewUserReturning(PrefService* prefs) {
  p3a_utils::RecordFeatureNewUserReturning(
      prefs, prefs::kBraveTodayFirstSessionTime,
      prefs::kBraveTodayLastSessionTime, prefs::kBraveTodayUsedSecondDay,
      kNewUserReturningHistogramName);
}

void RecordDaysInMonthUsedCount(PrefService* prefs, bool is_add) {
  p3a_utils::RecordFeatureDaysInMonthUsed(
      prefs, is_add, prefs::kBraveTodayLastSessionTime,
      prefs::kBraveTodayDaysInMonthUsedCount,
      kDaysInMonthUsedCountHistogramName);
}

void RecordWeeklySessionCount(PrefService* prefs, bool is_add) {
  // Track how many times in the past week
  // user has scrolled to Brave Today.
  constexpr int buckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  uint64_t total_session_count = AddToWeeklyStorageAndGetSum(
      prefs, prefs::kBraveTodayWeeklySessionCount, is_add);
  p3a_utils::RecordToHistogramBucket(kWeeklySessionCountHistogramName, buckets,
                                     total_session_count);
}

}  // namespace

NewsP3A::NewsP3A(PrefService* prefs)
    : prefs_(prefs),
      raw_session_card_view_count_(0),
      limited_session_card_view_count_(0),
      count_report_limiter_(kMaxViewRate,
                            kViewReportInterval,
                            base::BindRepeating(&NewsP3A::OnViewReportInterval,
                                                base::Unretained(this))) {}

NewsP3A::~NewsP3A() = default;

void NewsP3A::RecordAtSessionStart() {
  raw_session_card_view_count_ = 0;
  limited_session_card_view_count_ = 0;

  p3a_utils::RecordFeatureUsage(prefs_, prefs::kBraveTodayFirstSessionTime,
                                prefs::kBraveTodayLastSessionTime);

  RecordLastUsageTime(prefs_);
  RecordNewUserReturning(prefs_);
  RecordDaysInMonthUsedCount(prefs_, true);

  RecordWeeklySessionCount(prefs_, true);
}

void NewsP3A::RecordWeeklyMaxCardVisitsCount(
    uint64_t cards_visited_session_total_count) {
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  constexpr int buckets[] = {0, 1, 3, 6, 10, 15, 100};
  uint64_t max = UpdateWeeklyStorageWithValueAndGetMax(
      prefs_, prefs::kBraveTodayWeeklyCardVisitsCount,
      cards_visited_session_total_count);
  p3a_utils::RecordToHistogramBucket(kWeeklyMaxCardVisitsHistogramName, buckets,
                                     max);
}

void NewsP3A::RecordWeeklyMaxCardViewsCount(uint64_t new_card_views) {
  limited_session_card_view_count_ += new_card_views;
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  constexpr int buckets[] = {0, 1, 4, 12, 20, 40, 80, 1000};
  uint64_t max = UpdateWeeklyStorageWithValueAndGetMax(
      prefs_, prefs::kBraveTodayWeeklyCardViewsCount,
      limited_session_card_view_count_);
  p3a_utils::RecordToHistogramBucket(kWeeklyMaxCardViewsHistogramName, buckets,
                                     max);
}

void NewsP3A::RecordWeeklyDisplayAdsViewedCount(bool is_add) {
  // Store current weekly total in p3a, ready to send on the next upload
  constexpr int buckets[] = {0, 1, 4, 8, 14, 30, 60, 120};
  uint64_t total = AddToWeeklyStorageAndGetSum(
      prefs_, prefs::kBraveTodayWeeklyDisplayAdViewedCount, is_add);
  p3a_utils::RecordToHistogramBucket(kWeeklyDisplayAdsViewedHistogramName,
                                     buckets, total);
}

void NewsP3A::RecordDirectFeedsTotal() {
  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  const auto& direct_feeds_dict =
      prefs_->GetDict(prefs::kBraveTodayDirectFeeds);
  std::size_t feed_count = direct_feeds_dict.size();
  p3a_utils::RecordToHistogramBucket(kDirectFeedsTotalHistogramName, buckets,
                                     feed_count);
}

void NewsP3A::RecordCardViewMetrics(uint64_t cards_viewed_session_total_count) {
  DCHECK_GE(cards_viewed_session_total_count, raw_session_card_view_count_);
  uint64_t new_card_views =
      cards_viewed_session_total_count - raw_session_card_view_count_;
  raw_session_card_view_count_ = cards_viewed_session_total_count;
  count_report_limiter_.Add(new_card_views);
}

void NewsP3A::RecordWeeklyAddedDirectFeedsCount(int change) {
  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  uint64_t weekly_total = AddToWeeklyStorageAndGetSum(
      prefs_, prefs::kBraveTodayWeeklyAddedDirectFeedsCount, change);

  p3a_utils::RecordToHistogramBucket(kWeeklyAddedDirectFeedsHistogramName,
                                     buckets, weekly_total);
}

void NewsP3A::RecordTotalCardViews(uint64_t new_card_views) {
  WeeklyStorage total_storage(prefs_, prefs::kBraveTodayTotalCardViews);

  total_storage.AddDelta(new_card_views);

  uint64_t total = total_storage.GetWeeklySum();

  int buckets[] = {0, 1, 10, 20, 40, 80, 100};
  VLOG(1) << "NewsP3A: total card views update: total = " << total
          << " curr session = " << new_card_views;
  p3a_utils::RecordToHistogramBucket(kTotalCardViewsHistogramName, buckets,
                                     total);
}

void NewsP3A::OnViewReportInterval(uint64_t new_card_views) {
  RecordWeeklyMaxCardViewsCount(new_card_views);
  RecordTotalCardViews(new_card_views);
}

void NewsP3A::RecordAtInit() {
  RecordLastUsageTime(prefs_);
  RecordNewUserReturning(prefs_);
  RecordDaysInMonthUsedCount(prefs_, false);

  RecordDirectFeedsTotal();
  RecordWeeklyAddedDirectFeedsCount(0);
  RecordWeeklySessionCount(prefs_, false);
  RecordWeeklyMaxCardVisitsCount(0);
  RecordWeeklyMaxCardViewsCount(0);
  RecordWeeklyDisplayAdsViewedCount(false);
  RecordTotalCardViews(0);
}

void NewsP3A::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveTodayWeeklySessionCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardViewsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardVisitsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyDisplayAdViewedCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyAddedDirectFeedsCount);
  registry->RegisterListPref(prefs::kBraveTodayTotalCardViews);
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, prefs::kBraveTodayFirstSessionTime,
      prefs::kBraveTodayLastSessionTime, prefs::kBraveTodayUsedSecondDay,
      prefs::kBraveTodayDaysInMonthUsedCount);
}

}  // namespace p3a
}  // namespace brave_news
