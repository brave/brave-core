// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_p3a.h"

#include <algorithm>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_news {
namespace p3a {

namespace {

bool IsNewsEnabled(PrefService* prefs) {
  return prefs->GetBoolean(prefs::kBraveNewsOptedIn) &&
         prefs->GetBoolean(prefs::kNewTabPageShowToday);
}

uint64_t AddToWeeklyStorageAndGetSum(PrefService* prefs,
                                     const char* pref_name,
                                     int change) {
  WeeklyStorage storage(prefs, pref_name);
  if (change > 0) {
    storage.AddDelta(1);
  } else if (change < 0) {
    storage.SubDelta(1);
  }
  return storage.GetWeeklySum();
}

void RecordLastUsageTime(PrefService* prefs) {
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      prefs, prefs::kBraveNewsLastSessionTime, kLastUsageTimeHistogramName);
}

void RecordNewUserReturning(PrefService* prefs) {
  p3a_utils::RecordFeatureNewUserReturning(
      prefs, prefs::kBraveNewsFirstSessionTime,
      prefs::kBraveNewsLastSessionTime, prefs::kBraveNewsUsedSecondDay,
      kNewUserReturningHistogramName);
}

void RecordWeeklySessionCount(PrefService* prefs, bool is_add) {
  // Track how many times in the past week
  // user has scrolled to Brave News.
  constexpr int buckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  uint64_t total_session_count = AddToWeeklyStorageAndGetSum(
      prefs, prefs::kBraveNewsWeeklySessionCount, is_add);
  p3a_utils::RecordToHistogramBucket(kWeeklySessionCountHistogramName, buckets,
                                     total_session_count);
}

void RecordGeneralUsage() {
  UMA_HISTOGRAM_BOOLEAN(kUsageMonthlyHistogramName, true);
  UMA_HISTOGRAM_BOOLEAN(kUsageDailyHistogramName, true);
}

}  // namespace

void RecordAtSessionStart(PrefService* prefs) {
  p3a_utils::RecordFeatureUsage(prefs, prefs::kBraveNewsFirstSessionTime,
                                prefs::kBraveNewsLastSessionTime);

  RecordLastUsageTime(prefs);
  RecordNewUserReturning(prefs);
  RecordGeneralUsage();

  RecordWeeklySessionCount(prefs, true);
}

void RecordWeeklyDisplayAdsViewedCount(PrefService* prefs, bool is_add) {
  // Store current weekly total in p3a, ready to send on the next upload
  constexpr int buckets[] = {0, 1, 4, 8, 14, 30, 60, 120};
  uint64_t total = AddToWeeklyStorageAndGetSum(
      prefs, prefs::kBraveNewsWeeklyDisplayAdViewedCount, is_add);
  p3a_utils::RecordToHistogramBucket(kWeeklyDisplayAdsViewedHistogramName,
                                     buckets, total);
}

void RecordDirectFeedsTotal(PrefService* prefs) {
  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  const auto& direct_feeds_dict = prefs->GetDict(prefs::kBraveNewsDirectFeeds);
  std::size_t feed_count = direct_feeds_dict.size();
  p3a_utils::RecordToHistogramBucket(kDirectFeedsTotalHistogramName, buckets,
                                     feed_count);
}

void RecordWeeklyAddedDirectFeedsCount(PrefService* prefs, int change) {
  constexpr int buckets[] = {0, 1, 2, 3, 4, 5, 10};
  uint64_t weekly_total = AddToWeeklyStorageAndGetSum(
      prefs, prefs::kBraveNewsWeeklyAddedDirectFeedsCount, change);

  p3a_utils::RecordToHistogramBucket(kWeeklyAddedDirectFeedsHistogramName,
                                     buckets, weekly_total);
}

void RecordTotalCardViews(PrefService* prefs, uint64_t count_delta) {
  WeeklyStorage total_storage(prefs, prefs::kBraveNewsTotalCardViews);

  total_storage.AddDelta(count_delta);

  uint64_t total = total_storage.GetWeeklySum();

  int buckets[] = {0, 1, 10, 20, 40, 80, 100};
  VLOG(1) << "NewsP3A: total card views update: total = " << total
          << " count delta = " << count_delta;
  p3a_utils::RecordToHistogramBucket(kTotalCardViewsHistogramName, buckets,
                                     total);
}

void RecordFeatureEnabledChange(PrefService* prefs) {
  bool enabled = IsNewsEnabled(prefs);
  bool was_ever_enabled = prefs->GetBoolean(prefs::kBraveNewsWasEverEnabled);
  if (!enabled && !was_ever_enabled) {
    // If the user clicked "no thanks" on the NTP, then we don't want to record
    // this as an opt-out, since they were never opted in.
    return;
  }
  prefs->SetBoolean(prefs::kBraveNewsWasEverEnabled, true);
  UMA_HISTOGRAM_BOOLEAN(kIsEnabledHistogramName, enabled);
}

void RecordAtInit(PrefService* prefs) {
  RecordLastUsageTime(prefs);
  RecordNewUserReturning(prefs);

  RecordDirectFeedsTotal(prefs);
  RecordWeeklyAddedDirectFeedsCount(prefs, 0);
  RecordWeeklySessionCount(prefs, false);
  RecordWeeklyDisplayAdsViewedCount(prefs, false);
  RecordTotalCardViews(prefs, 0);

  if (IsNewsEnabled(prefs)) {
    prefs->SetBoolean(prefs::kBraveNewsWasEverEnabled, true);
  }
}

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveNewsWeeklySessionCount);
  registry->RegisterListPref(prefs::kBraveNewsWeeklyDisplayAdViewedCount);
  registry->RegisterListPref(prefs::kBraveNewsWeeklyAddedDirectFeedsCount);
  registry->RegisterListPref(prefs::kBraveNewsTotalCardViews);
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, prefs::kBraveNewsFirstSessionTime,
      prefs::kBraveNewsLastSessionTime, prefs::kBraveNewsUsedSecondDay, nullptr,
      nullptr);
  registry->RegisterBooleanPref(prefs::kBraveNewsWasEverEnabled, false);
}

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  // added 05/2023
  registry->RegisterListPref(prefs::kBraveNewsWeeklyCardViewsCount);
  registry->RegisterListPref(prefs::kBraveNewsWeeklyCardVisitsCount);
  registry->RegisterUint64Pref(prefs::kBraveNewsCurrSessionCardViews, 0);
  registry->RegisterListPref(prefs::kBraveNewsDaysInMonthUsedCount);
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // added 05/2023
  prefs->ClearPref(prefs::kBraveNewsWeeklyCardViewsCount);
  prefs->ClearPref(prefs::kBraveNewsWeeklyCardVisitsCount);
  prefs->ClearPref(prefs::kBraveNewsCurrSessionCardViews);
  prefs->ClearPref(prefs::kBraveNewsDaysInMonthUsedCount);
}

}  // namespace p3a
}  // namespace brave_news
