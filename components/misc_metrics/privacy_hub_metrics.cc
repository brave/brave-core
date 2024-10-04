/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/privacy_hub_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {
const int kViewsMonthlyBucketValues[] = {1, 10, 20};
constexpr base::TimeDelta kReportUpdateInterval = base::Days(1);
}  // namespace


PrivacyHubMetrics::PrivacyHubMetrics(PrefService* local_state)
    : view_storage_(local_state, kMiscMetricsPrivacyHubViews) {
  SetUpTimer();
}

PrivacyHubMetrics::~PrivacyHubMetrics() = default;

void PrivacyHubMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsPrivacyHubViews);
}

void PrivacyHubMetrics::RecordView() {
  view_storage_.AddDelta(1u);
  RecordViewCount();
}

void PrivacyHubMetrics::RecordEnabledStatus(bool is_enabled) {
  // suspend metric if not enabled; we only want to report
  // if the feature is enabled
  int histogram_value = is_enabled ? 1 : INT_MAX - 1;
  UMA_HISTOGRAM_EXACT_LINEAR(kIsEnabledHistogramName, histogram_value, 2);
}

void PrivacyHubMetrics::RecordViewCount() {
  auto sum = view_storage_.GetMonthlySum();
  if (sum > 0) {
    p3a_utils::RecordToHistogramBucket(kViewsMonthlyHistogramName,
                                       kViewsMonthlyBucketValues, sum);
  }
  SetUpTimer();
}

void PrivacyHubMetrics::SetUpTimer() {
  report_timer_.Start(FROM_HERE, base::Time::Now() + kReportUpdateInterval,
                      base::BindOnce(&PrivacyHubMetrics::RecordViewCount,
                                     base::Unretained(this)));
}

}  // namespace misc_metrics
