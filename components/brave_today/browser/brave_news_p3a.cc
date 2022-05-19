// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_p3a.h"

#include <algorithm>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/time_period_storage/monthly_storage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_news {
namespace p3a {

namespace {

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

template <std::size_t N>
void RecordToHistogramBucket(const char* histogram_name,
                             const int (&buckets)[N],
                             uint64_t value) {
  const int* it_count = std::lower_bound(buckets, std::end(buckets), value);
  int answer = it_count - buckets;
  base::UmaHistogramExactLinear(histogram_name, answer, std::size(buckets) + 1);
}

void RecordLastUsageTime(PrefService* prefs) {
  base::Time last_session_time =
      prefs->GetTime(prefs::kBraveTodayLastSessionTime);
  if (last_session_time.is_null()) {
    return;
  }
  int answer = 0;
  int duration_days = (base::Time::Now() - last_session_time).InDays();
  int duration_weeks = duration_days / 7;
  if (duration_weeks < 4) {
    answer = duration_weeks + 1;
  } else {
    int duration_months = duration_days / 30;
    answer = duration_months < 2 ? 5 : 6;
  }
  UMA_HISTOGRAM_EXACT_LINEAR(kLastUsageTimeHistogramName, answer, 7);
}

void RecordNewUserReturning(PrefService* prefs) {
  base::Time last_use_time = prefs->GetTime(prefs::kBraveTodayLastSessionTime);
  base::Time first_use_time =
      prefs->GetTime(prefs::kBraveTodayFirstSessionTime);
  int answer = 0;
  if (!first_use_time.is_null()) {
    // If the first use time was set (by RecordAtSessionStart),
    // we can assume that News was used at least once
    bool prev_used_second_day =
        prefs->GetBoolean(prefs::kBraveTodayUsedSecondDay);
    int first_now_delta_days = (base::Time::Now() - first_use_time).InDays();
    int first_last_delta_days = (last_use_time - first_use_time).InDays();
    if (first_now_delta_days >= 7) {
      // I used Brave News, but I'm not a first time user this week
      answer = 1;
    } else if (first_last_delta_days == 0) {
      // I'm a first time user this week, but did not return again during the
      // week
      answer = 2;
    } else if (prev_used_second_day || first_last_delta_days == 1) {
      // I'm a first time user this week, and I returned the following day
      answer = 3;
      if (!prev_used_second_day) {
        // Set a preference flag to ensure that the same answer
        // is recorded for the rest of the week
        prefs->SetBoolean(prefs::kBraveTodayUsedSecondDay, true);
      }
    } else {
      // I'm a first time user this week, and returned this week (but not the
      // following day)
      answer = 4;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR(kNewUserReturningHistogramName, answer, 5);
}

void RecordDaysInMonthUsedCount(PrefService* prefs, bool is_add) {
  if (prefs->GetTime(prefs::kBraveTodayLastSessionTime).is_null()) {
    // Don't report if News was never used
    return;
  }
  // How many days was News used in the last month?
  constexpr int buckets[] = {0, 1, 2, 5, 10, 15, 20, 100};
  MonthlyStorage storage(prefs, prefs::kBraveTodayDaysInMonthUsedCount);
  if (is_add) {
    storage.ReplaceTodaysValueIfGreater(1);
  }
  RecordToHistogramBucket(kDaysInMonthUsedCountHistogramName, buckets,
                          storage.GetMonthlySum());
}

void RecordWeeklySessionCount(PrefService* prefs, bool is_add) {
  // Track how many times in the past week
  // user has scrolled to Brave Today.
  constexpr int buckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  uint64_t total_session_count = AddToWeeklyStorageAndGetSum(
      prefs, prefs::kBraveTodayWeeklySessionCount, is_add);
  RecordToHistogramBucket(kWeeklySessionCountHistogramName, buckets,
                          total_session_count);
}

void ResetCurrSessionTotalViewsCount(PrefService* prefs) {
  prefs->SetUint64(prefs::kBraveTodayCurrSessionCardViews, 0);
  VLOG(1) << "NewsP3A: reset curr session total card views count";
}

}  // namespace

void RecordAtSessionStart(PrefService* prefs) {
  base::Time now_midnight = base::Time::Now().LocalMidnight();
  prefs->SetTime(prefs::kBraveTodayLastSessionTime, now_midnight);
  if (prefs->GetTime(prefs::kBraveTodayFirstSessionTime).is_null()) {
    prefs->SetTime(prefs::kBraveTodayFirstSessionTime, now_midnight);
  }
  RecordLastUsageTime(prefs);
  RecordNewUserReturning(prefs);
  RecordDaysInMonthUsedCount(prefs, true);

  RecordWeeklySessionCount(prefs, true);
  ResetCurrSessionTotalViewsCount(prefs);
}

void RecordWeeklyMaxCardVisitsCount(
    PrefService* prefs,
    uint64_t cards_visited_session_total_count) {
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  constexpr int buckets[] = {0, 1, 3, 6, 10, 15, 100};
  uint64_t max = UpdateWeeklyStorageWithValueAndGetMax(
      prefs, prefs::kBraveTodayWeeklyCardVisitsCount,
      cards_visited_session_total_count);
  RecordToHistogramBucket(kWeeklyMaxCardVisitsHistogramName, buckets, max);
}

void RecordWeeklyMaxCardViewsCount(PrefService* prefs,
                                   uint64_t cards_viewed_session_total_count) {
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  constexpr int buckets[] = {0, 1, 4, 12, 20, 40, 80, 1000};
  uint64_t max = UpdateWeeklyStorageWithValueAndGetMax(
      prefs, prefs::kBraveTodayWeeklyCardViewsCount,
      cards_viewed_session_total_count);
  RecordToHistogramBucket(kWeeklyMaxCardViewsHistogramName, buckets, max);
}

void RecordWeeklyDisplayAdsViewedCount(PrefService* prefs, bool is_add) {
  // Store current weekly total in p3a, ready to send on the next upload
  constexpr int buckets[] = {0, 1, 4, 8, 14, 30, 60, 120};
  uint64_t total = AddToWeeklyStorageAndGetSum(
      prefs, prefs::kBraveTodayWeeklyCardViewsCount, is_add);
  RecordToHistogramBucket(kWeeklyDisplayAdsViewedHistogramName, buckets, total);
}

void RecordDirectFeedsTotal(PrefService* prefs) {
  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  const base::Value* direct_feeds_dict =
      prefs->GetDictionary(prefs::kBraveTodayDirectFeeds);
  DCHECK(direct_feeds_dict && direct_feeds_dict->is_dict());
  std::size_t feed_count = direct_feeds_dict->DictSize();
  RecordToHistogramBucket(kDirectFeedsTotalHistogramName, buckets, feed_count);
}

void RecordWeeklyAddedDirectFeedsCount(PrefService* prefs, int change) {
  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  uint64_t weekly_total = AddToWeeklyStorageAndGetSum(
      prefs, prefs::kBraveTodayWeeklyAddedDirectFeedsCount, change);

  RecordToHistogramBucket(kWeeklyAddedDirectFeedsHistogramName, buckets,
                          weekly_total);
}

void RecordTotalCardViews(PrefService* prefs,
                          uint64_t cards_viewed_session_total_count) {
  WeeklyStorage total_storage(prefs, prefs::kBraveTodayTotalCardViews);

  uint64_t stored_curr_session_views =
      prefs->GetUint64(prefs::kBraveTodayCurrSessionCardViews);

  // Since the front-end repeatedly sends the updated total,
  // we should subtract the last known total for the current session and
  // add the new total.
  total_storage.SubDelta(stored_curr_session_views);
  total_storage.AddDelta(cards_viewed_session_total_count);

  prefs->SetUint64(prefs::kBraveTodayCurrSessionCardViews,
                   cards_viewed_session_total_count);

  uint64_t total = total_storage.GetWeeklySum();

  int buckets[] = {0, 1, 10, 20, 40, 80, 100};
  VLOG(1) << "NewsP3A: total card views update: total = " << total
          << " curr session = " << cards_viewed_session_total_count;
  RecordToHistogramBucket(kTotalCardViewsHistogramName, buckets, total);
}

void RecordAtInit(PrefService* prefs) {
  ResetCurrSessionTotalViewsCount(prefs);

  RecordLastUsageTime(prefs);
  RecordNewUserReturning(prefs);
  RecordDaysInMonthUsedCount(prefs, false);

  RecordDirectFeedsTotal(prefs);
  RecordWeeklyAddedDirectFeedsCount(prefs, 0);
  RecordWeeklySessionCount(prefs, false);
  RecordWeeklyMaxCardVisitsCount(prefs, 0);
  RecordWeeklyMaxCardViewsCount(prefs, 0);
  RecordWeeklyDisplayAdsViewedCount(prefs, false);
  RecordTotalCardViews(prefs, 0);
}

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveTodayWeeklySessionCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardViewsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardVisitsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyDisplayAdViewedCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyAddedDirectFeedsCount);
  registry->RegisterListPref(prefs::kBraveTodayTotalCardViews);
  registry->RegisterListPref(prefs::kBraveTodayDaysInMonthUsedCount);
  registry->RegisterUint64Pref(prefs::kBraveTodayCurrSessionCardViews, 0);
  registry->RegisterTimePref(prefs::kBraveTodayFirstSessionTime, base::Time());
  registry->RegisterTimePref(prefs::kBraveTodayLastSessionTime, base::Time());
  registry->RegisterBooleanPref(prefs::kBraveTodayUsedSecondDay, false);
}

}  // namespace p3a
}  // namespace brave_news
