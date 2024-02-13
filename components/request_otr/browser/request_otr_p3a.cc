/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_p3a.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "brave/components/time_period_storage/monthly_storage.h"
#include "components/prefs/pref_registry_simple.h"

namespace request_otr {
namespace p3a {

namespace {

constexpr int kUsageCountBuckets[] = {0, 1, 2, 5, 10};
constexpr int kDurationBuckets[] = {5, 10, 15, 30, 60};

}  // namespace

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kInterstitialShownStorage);
  registry->RegisterListPref(kInterstitialDurationStorage);
  registry->RegisterListPref(kSessionCountStorage);
}

void RecordSessionCount(PrefService* prefs, bool new_session_started) {
  MonthlyStorage session_count_storage(prefs, kSessionCountStorage);

  if (new_session_started) {
    session_count_storage.AddDelta(1);
  }

  p3a_utils::RecordToHistogramBucket(kSessionCountHistogramName,
                                     kUsageCountBuckets,
                                     session_count_storage.GetMonthlySum());
}

void RecordInterstitialShown(PrefService* prefs, bool new_page_shown) {
  MonthlyStorage shown_storage(prefs, kInterstitialShownStorage);

  if (new_page_shown) {
    shown_storage.AddDelta(1);
  }

  p3a_utils::RecordToHistogramBucket(kInterstitialShownHistogramName,
                                     kUsageCountBuckets,
                                     shown_storage.GetMonthlySum());
}

void RecordInterstitialEnd(PrefService* prefs, base::Time new_page_start_time) {
  MonthlyStorage count_storage(prefs, kInterstitialShownStorage);
  MonthlyStorage duration_storage(prefs, kInterstitialDurationStorage);

  if (!new_page_start_time.is_null()) {
    duration_storage.AddDelta(
        (base::Time::Now() - new_page_start_time).InSeconds());
  }

  uint64_t month_duration_sum = duration_storage.GetMonthlySum();
  uint64_t month_count_sum = count_storage.GetMonthlySum();

  if (month_count_sum > 0) {
    uint64_t avg_duration = month_duration_sum / month_count_sum;
    p3a_utils::RecordToHistogramBucket(kInterstitialDurationHistogramName,
                                       kDurationBuckets, avg_duration);
  }
}

void UpdateMetrics(PrefService* prefs) {
  RecordSessionCount(prefs, false);
  RecordInterstitialShown(prefs, false);
  RecordInterstitialEnd(prefs, {});
}

}  // namespace p3a
}  // namespace request_otr
