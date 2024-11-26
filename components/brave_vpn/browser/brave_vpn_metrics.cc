/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

namespace {

constexpr int kWidgetUsageBuckets[] = {1, 10, 20};

}  // namespace

BraveVpnMetrics::BraveVpnMetrics(PrefService* local_state,
                                 PrefService* profile_prefs)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      widget_usage_storage_(local_state_,
                            prefs::kBraveVPNWidgetUsageWeeklyStorage) {
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(
      kNewTabPageShowBraveVPN,
      base::BindRepeating(&BraveVpnMetrics::HandleShowWidgetChange,
                          base::Unretained(this)));
  RecordAllMetrics(false);
}

BraveVpnMetrics::~BraveVpnMetrics() = default;

void BraveVpnMetrics::RecordAllMetrics(bool new_usage) {
  if (new_usage) {
    p3a_utils::RecordFeatureUsage(local_state_, prefs::kBraveVPNFirstUseTime,
                                  prefs::kBraveVPNLastUseTime);
  }
  p3a_utils::RecordFeatureNewUserReturning(
      local_state_, prefs::kBraveVPNFirstUseTime, prefs::kBraveVPNLastUseTime,
      prefs::kBraveVPNUsedSecondDay, kNewUserReturningHistogramName);
  p3a_utils::RecordFeatureDaysInMonthUsed(
      local_state_, new_usage, prefs::kBraveVPNLastUseTime,
      prefs::kBraveVPNDaysInMonthUsed, kDaysInMonthUsedHistogramName);
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, prefs::kBraveVPNLastUseTime, kLastUsageTimeHistogramName);
  RecordWidgetUsage(false);
  report_timer_.Start(FROM_HERE,
                      base::Time::Now() + base::Hours(kP3AIntervalHours),
                      base::BindOnce(&BraveVpnMetrics::RecordAllMetrics,
                                     base::Unretained(this), false));
}

#if BUILDFLAG(IS_ANDROID)
void BraveVpnMetrics::RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                                 int64_t session_end_time_ms) {
  if (session_start_time_ms < 0 || session_end_time_ms < 0) {
    RecordAllMetrics(false);
    return;
  }
  base::Time session_start_time =
      base::Time::FromMillisecondsSinceUnixEpoch(
          static_cast<double>(session_start_time_ms))
          .LocalMidnight();
  base::Time session_end_time = base::Time::FromMillisecondsSinceUnixEpoch(
                                    static_cast<double>(session_end_time_ms))
                                    .LocalMidnight();
  for (base::Time day = session_start_time; day <= session_end_time;
       day += base::Days(1)) {
    bool is_last_day = day == session_end_time;
    // Call functions for each day in the last session to ensure
    // p3a_util functions produce the correct result
    p3a_utils::RecordFeatureUsage(local_state_, prefs::kBraveVPNFirstUseTime,
                                  prefs::kBraveVPNLastUseTime, day);
    p3a_utils::RecordFeatureNewUserReturning(
        local_state_, prefs::kBraveVPNFirstUseTime, prefs::kBraveVPNLastUseTime,
        prefs::kBraveVPNUsedSecondDay, kNewUserReturningHistogramName,
        is_last_day);
    p3a_utils::RecordFeatureDaysInMonthUsed(
        local_state_, day, prefs::kBraveVPNLastUseTime,
        prefs::kBraveVPNDaysInMonthUsed, kDaysInMonthUsedHistogramName,
        is_last_day);
  }
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, prefs::kBraveVPNLastUseTime, kLastUsageTimeHistogramName);
}
#endif

void BraveVpnMetrics::RecordWidgetUsage(bool new_usage) {
  if (new_usage) {
    widget_usage_storage_.AddDelta(1u);
  }
  auto total = widget_usage_storage_.GetWeeklySum();
  if (total == 0) {
    return;
  }
  p3a_utils::RecordToHistogramBucket(kWidgetUsageHistogramName,
                                     kWidgetUsageBuckets, total);
}

void BraveVpnMetrics::HandleShowWidgetChange() {
  if (profile_prefs_->GetBoolean(kNewTabPageShowBraveVPN)) {
    return;
  }
  UMA_HISTOGRAM_BOOLEAN(kHideWidgetHistogramName, true);
}

}  // namespace brave_vpn
