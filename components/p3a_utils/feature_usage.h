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

void RegisterFeatureUsagePrefs(PrefRegistrySimple* registry,
                               const char* first_use_time_pref_name,
                               const char* last_use_time_pref_name,
                               const char* used_second_day_pref_name,
                               const char* days_in_month_used_pref_name);

void RecordFeatureUsage(PrefService* prefs,
                        const char* first_use_time_pref_name,
                        const char* last_use_time_pref_name);

void RecordFeatureNewUserReturning(PrefService* prefs,
                                   const char* first_use_time_pref_name,
                                   const char* last_use_time_pref_name,
                                   const char* used_second_day_pref_name,
                                   const char* histogram_name);

void RecordFeatureDaysInMonthUsed(PrefService* prefs,
                                  bool is_add,
                                  const char* last_use_time_pref_name,
                                  const char* days_in_month_used_pref_name,
                                  const char* histogram_name);

void RecordFeatureLastUsageTimeMetric(PrefService* prefs,
                                      const char* last_use_time_pref_name,
                                      const char* histogram_name);

}  // namespace p3a_utils

#endif  // BRAVE_COMPONENTS_P3A_UTILS_FEATURE_USAGE_H_
