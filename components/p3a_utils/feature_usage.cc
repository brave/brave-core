/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a_utils/feature_usage.h"

#include "base/metrics/histogram_functions.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/monthly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace p3a_utils {

void RegisterFeatureUsagePrefs(PrefRegistrySimple* registry,
                               const char* first_use_time_pref_name,
                               const char* last_use_time_pref_name,
                               const char* used_second_day_pref_name,
                               const char* days_in_month_used_pref_name) {
  if (first_use_time_pref_name) {
    registry->RegisterTimePref(first_use_time_pref_name, base::Time());
  }
  if (last_use_time_pref_name) {
    registry->RegisterTimePref(last_use_time_pref_name, base::Time());
  }
  if (used_second_day_pref_name) {
    registry->RegisterBooleanPref(used_second_day_pref_name, false);
  }
  if (days_in_month_used_pref_name) {
    registry->RegisterListPref(days_in_month_used_pref_name);
  }
}

void RecordFeatureUsage(PrefService* prefs,
                        const char* first_use_time_pref_name,
                        const char* last_use_time_pref_name) {
  base::Time now_midnight = base::Time::Now().LocalMidnight();
  prefs->SetTime(last_use_time_pref_name, now_midnight);
  if (prefs->GetTime(first_use_time_pref_name).is_null()) {
    prefs->SetTime(first_use_time_pref_name, now_midnight);
  }
}

void RecordFeatureNewUserReturning(PrefService* prefs,
                                   const char* first_use_time_pref_name,
                                   const char* last_use_time_pref_name,
                                   const char* used_second_day_pref_name,
                                   const char* histogram_name) {
  base::Time last_use_time = prefs->GetTime(last_use_time_pref_name);
  base::Time first_use_time = prefs->GetTime(first_use_time_pref_name);
  int answer = 0;
  if (!first_use_time.is_null()) {
    // If the first use time was set, we can assume that
    // the feature was used at least once
    bool prev_used_second_day = prefs->GetBoolean(used_second_day_pref_name);
    int first_now_delta_days = (base::Time::Now() - first_use_time).InDays();
    int first_last_delta_days = (last_use_time - first_use_time).InDays();
    if (first_now_delta_days >= 7) {
      // I used the feature, but I'm not a first time user this week
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
        prefs->SetBoolean(used_second_day_pref_name, true);
      }
    } else {
      // I'm a first time user this week, and returned this week (but not the
      // following day)
      answer = 4;
    }
  }
  base::UmaHistogramExactLinear(histogram_name, answer, 5);
}

void RecordFeatureDaysInMonthUsed(PrefService* prefs,
                                  bool is_add,
                                  const char* last_use_time_pref_name,
                                  const char* days_in_month_used_pref_name,
                                  const char* histogram_name) {
  if (prefs->GetTime(last_use_time_pref_name).is_null()) {
    // Don't report if News was never used
    return;
  }
  // How many days was News used in the last month?
  constexpr int buckets[] = {0, 1, 2, 5, 10, 15, 20, 100};
  MonthlyStorage storage(prefs, days_in_month_used_pref_name);
  if (is_add) {
    storage.ReplaceTodaysValueIfGreater(1);
  }
  RecordToHistogramBucket(histogram_name, buckets, storage.GetMonthlySum());
}

void RecordFeatureLastUsageTimeMetric(PrefService* prefs,
                                      const char* last_use_time_pref_name,
                                      const char* histogram_name) {
  base::Time last_use_time = prefs->GetTime(last_use_time_pref_name);
  if (last_use_time.is_null()) {
    return;
  }
  int answer = 0;
  int duration_days = (base::Time::Now() - last_use_time).InDays();
  int duration_weeks = duration_days / 7;
  if (duration_weeks < 4) {
    answer = duration_weeks + 1;
  } else {
    int duration_months = duration_days / 30;
    answer = duration_months < 2 ? 5 : 6;
  }
  base::UmaHistogramExactLinear(histogram_name, answer, 7);
}

}  // namespace p3a_utils
