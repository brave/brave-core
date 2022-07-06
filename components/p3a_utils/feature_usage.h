/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_UTILS_FEATURE_USAGE_H_
#define BRAVE_COMPONENTS_P3A_UTILS_FEATURE_USAGE_H_

#include "base/time/time.h"

class PrefRegistrySimple;
class PrefService;

namespace p3a_utils {

// Registers preferences required for managing common feature usage metrics.
// first_use_time_pref_name and last_use_time_pref_name are required.
// used_second_day_pref_name is only required if using the NewUserReturning
// metric. days_in_month_used_pref_name is only required if using the
// DaysInMonth metric.
void RegisterFeatureUsagePrefs(PrefRegistrySimple* registry,
                               const char* first_use_time_pref_name,
                               const char* last_use_time_pref_name,
                               const char* used_second_day_pref_name,
                               const char* days_in_month_used_pref_name);

// Updates the first/last use time preferences which will be used by
// the metric recording functions below. To be called for each usage of the
// relevant feature.
void RecordFeatureUsage(PrefService* prefs,
                        const char* first_use_time_pref_name,
                        const char* last_use_time_pref_name);

// Assists in migrating existing external last use timestamps that existed
// before this P3A utility was created.
void MaybeRecordFeatureExistingUsageTimestamp(
    PrefService* prefs,
    const char* first_use_time_pref_name,
    const char* last_use_time_pref_name,
    base::Time external_last_use_timestamp);

// Records the NewUserReturning metric.
//
// Question: As a first time user of the feature this week,
//           did I return again to use it during the week?
// Answers:
// 0. I have never used the feature
// 1. I have used the feature, but I'm not a first time
//    feature user this week
// 2. I'm a first time feature user this week but, no,
//    I did not return the rest of the week
// 3. I'm a first time feature this week and, yes,
//    I returned and used it again the following day
// 4. I'm a first time feature user this week and,
//    yes, I returned and used it again this week but not the following day
void RecordFeatureNewUserReturning(PrefService* prefs,
                                   const char* first_use_time_pref_name,
                                   const char* last_use_time_pref_name,
                                   const char* used_second_day_pref_name,
                                   const char* histogram_name);

// Records the DaysInMonthUsed metric. Will not report if feature never used.
//
// Question: As an opted in feature user, how many days did I
//           use the feature in the last 30 days?
// Answers:
// 0. 0 days
// 1. 1 day
// 2. 2 days
// 3. 3 to 5 days
// 4. 6 to 10 days
// 5. 11 to 15 days
// 6. 16 to 20 days
// 7. More than 20 days
void RecordFeatureDaysInMonthUsed(PrefService* prefs,
                                  bool is_add,
                                  const char* last_use_time_pref_name,
                                  const char* days_in_month_used_pref_name,
                                  const char* histogram_name);

// Records the LastUsageTime metric. Will not report if feature never used.
//
// Question: As an opted in feature user, when was the last time I used the
// feature?
//
// Answers:
// 1. This week
// 2. 1 week ago
// 3. 2 weeks ago
// 4. 3 weeks ago
// 5. A month ago
// 6. Over a month ago
void RecordFeatureLastUsageTimeMetric(PrefService* prefs,
                                      const char* last_use_time_pref_name,
                                      const char* histogram_name);

}  // namespace p3a_utils

#endif  // BRAVE_COMPONENTS_P3A_UTILS_FEATURE_USAGE_H_
