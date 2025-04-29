/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a_utils/feature_usage.h"

#include "base/metrics/histogram_functions.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/monthly_storage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace p3a_utils {

namespace {

constexpr int kDaysInMonthBuckets[] = {0, 1, 2, 5, 10, 15, 20, 100};
constexpr int kDaysInWeekBuckets[] = {0, 2, 4, 6};

}  // namespace

void RegisterFeatureUsagePrefs(PrefRegistrySimple* registry,
                               const char* first_use_time_pref_name,
                               const char* last_use_time_pref_name,
                               const char* used_second_day_pref_name,
                               const char* days_in_month_used_pref_name,
                               const char* days_in_week_used_pref_name) {
  DCHECK(registry);

  if (first_use_time_pref_name != nullptr) {
    registry->RegisterTimePref(first_use_time_pref_name, base::Time());
  }
  if (last_use_time_pref_name != nullptr) {
    registry->RegisterTimePref(last_use_time_pref_name, base::Time());
  }

  if (used_second_day_pref_name != nullptr) {
    registry->RegisterBooleanPref(used_second_day_pref_name, false);
  }
  if (days_in_month_used_pref_name != nullptr) {
    registry->RegisterListPref(days_in_month_used_pref_name);
  }
  if (days_in_week_used_pref_name != nullptr) {
    registry->RegisterListPref(days_in_week_used_pref_name);
  }
}

void RecordFeatureUsage(PrefService* prefs,
                        const char* first_use_time_pref_name,
                        const char* last_use_time_pref_name) {
  RecordFeatureUsage(prefs, first_use_time_pref_name, last_use_time_pref_name,
                     base::Time::Now());
}

void RecordFeatureUsage(PrefService* prefs,
                        const char* first_use_time_pref_name,
                        const char* last_use_time_pref_name,
                        const base::Time& last_new_use_time) {
  DCHECK(prefs);
  DCHECK(last_use_time_pref_name);
  DCHECK(!last_new_use_time.is_null());

  base::Time new_time_midnight = last_new_use_time.LocalMidnight();
  prefs->SetTime(last_use_time_pref_name, new_time_midnight);
  if (first_use_time_pref_name) {
    if (prefs->GetTime(first_use_time_pref_name).is_null()) {
      prefs->SetTime(first_use_time_pref_name, new_time_midnight);
    }
  }
}

void MaybeRecordFeatureExistingUsageTimestamp(
    PrefService* prefs,
    const char* first_use_time_pref_name,
    const char* last_use_time_pref_name,
    base::Time external_last_use_timestamp) {
  DCHECK(prefs);
  DCHECK(first_use_time_pref_name);
  DCHECK(last_use_time_pref_name);

  if (!prefs->GetTime(first_use_time_pref_name).is_null() ||
      external_last_use_timestamp.is_null()) {
    return;
  }
  // If first use time is null and external ts is not,
  // set the last use time to the external ts so the user does not appear new
  // in the "new user returning" metric
  prefs->SetTime(first_use_time_pref_name,
                 external_last_use_timestamp - base::Days(90));
  prefs->SetTime(last_use_time_pref_name, external_last_use_timestamp);
}

void RecordFeatureNewUserReturning(PrefService* prefs,
                                   const char* first_use_time_pref_name,
                                   const char* last_use_time_pref_name,
                                   const char* used_second_day_pref_name,
                                   const char* histogram_name,
                                   bool write_to_histogram,
                                   bool active_users_only) {
  DCHECK(prefs);
  DCHECK(first_use_time_pref_name);
  DCHECK(last_use_time_pref_name);
  DCHECK(used_second_day_pref_name);
  DCHECK(histogram_name);

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
  if (write_to_histogram) {
    if (active_users_only && answer == 0) {
      // Skip reporting if not an active user.
      return;
    }
    base::UmaHistogramExactLinear(histogram_name, answer, 5);
  }
}

void RecordFeatureDaysInMonthUsed(PrefService* prefs,
                                  const base::Time& add_date,
                                  const char* last_use_time_pref_name,
                                  const char* days_in_month_used_pref_name,
                                  const char* histogram_name,
                                  bool write_to_histogram) {
  DCHECK(prefs);
  DCHECK(last_use_time_pref_name);
  DCHECK(days_in_month_used_pref_name);
  DCHECK(histogram_name);

  if (prefs->GetTime(last_use_time_pref_name).is_null()) {
    // Don't report if feature was never used
    return;
  }
  MonthlyStorage storage(prefs, days_in_month_used_pref_name);
  if (!add_date.is_null()) {
    storage.ReplaceIfGreaterForDate(add_date, 1);
  }
  if (write_to_histogram) {
    RecordToHistogramBucket(histogram_name, kDaysInMonthBuckets,
                            storage.GetMonthlySum());
  }
}

void RecordFeatureDaysInMonthUsed(PrefService* prefs,
                                  bool is_add,
                                  const char* last_use_time_pref_name,
                                  const char* days_in_month_used_pref_name,
                                  const char* histogram_name,
                                  bool write_to_histogram) {
  RecordFeatureDaysInMonthUsed(
      prefs, is_add ? base::Time::Now() : base::Time(), last_use_time_pref_name,
      days_in_month_used_pref_name, histogram_name, write_to_histogram);
}

void RecordFeatureDaysInWeekUsed(PrefService* prefs,
                                 bool is_add,
                                 const char* days_in_week_used_pref_name,
                                 const char* histogram_name) {
  DCHECK(prefs);
  DCHECK(days_in_week_used_pref_name);
  DCHECK(histogram_name);

  WeeklyStorage storage(prefs, days_in_week_used_pref_name);
  if (is_add) {
    storage.ReplaceTodaysValueIfGreater(1);
  }

  auto sum = storage.GetWeeklySum();
  if (sum == 0) {
    return;
  }

  RecordToHistogramBucket(histogram_name, kDaysInWeekBuckets, sum);
}

void RecordFeatureLastUsageTimeMetric(PrefService* prefs,
                                      const char* last_use_time_pref_name,
                                      const char* histogram_name,
                                      bool single_month_only) {
  DCHECK(prefs);
  DCHECK(last_use_time_pref_name);
  DCHECK(histogram_name);

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
    if (single_month_only) {
      if (duration_days <= 30) {
        answer = 4;
      } else {
        // Do not record if past 30 days.
        return;
      }
    } else {
      int duration_months = duration_days / 30;
      answer = duration_months < 2 ? 5 : 6;
    }
  }

  base::UmaHistogramExactLinear(histogram_name, answer, 7);
}

}  // namespace p3a_utils
