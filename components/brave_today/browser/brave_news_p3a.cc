// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_p3a.h"

#include <algorithm>

#include "base/cxx17_backports.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

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
  base::UmaHistogramExactLinear(histogram_name, answer,
                                base::size(buckets) + 1);
}

}  // namespace

namespace brave_news {
namespace p3a {

void RecordEverInteracted() {
  // Track if user has ever scrolled to Brave Today.
  UMA_HISTOGRAM_EXACT_LINEAR(kEverInteractedHistogramName, 1, 1);
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

void RecordAtStart(PrefService* prefs) {
  RecordDirectFeedsTotal(prefs);
  RecordWeeklyAddedDirectFeedsCount(prefs, 0);
  RecordWeeklySessionCount(prefs, false);
  RecordWeeklyMaxCardVisitsCount(prefs, 0);
  RecordWeeklyMaxCardViewsCount(prefs, 0);
  RecordWeeklyDisplayAdsViewedCount(prefs, false);
}

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveTodayWeeklySessionCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardViewsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardVisitsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyDisplayAdViewedCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyAddedDirectFeedsCount);
}

}  // namespace p3a
}  // namespace brave_news
