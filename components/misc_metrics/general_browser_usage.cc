/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/general_browser_usage.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/iso_weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

const base::TimeDelta kReportInterval = base::Days(1);

#if !BUILDFLAG(IS_ANDROID)
constexpr int kProfileCountBuckets[] = {0, 1, 2, 3, 5};
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace

GeneralBrowserUsage::GeneralBrowserUsage(PrefService* local_state) {
  usage_storage_ = std::make_unique<ISOWeeklyStorage>(
      local_state, kMiscMetricsBrowserUsageList);
  Update();
}

GeneralBrowserUsage::~GeneralBrowserUsage() = default;

void GeneralBrowserUsage::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsBrowserUsageList);
}

void GeneralBrowserUsage::ReportWeeklyUse() {
  usage_storage_->ReplaceTodaysValueIfGreater(1);
  UMA_HISTOGRAM_EXACT_LINEAR(kWeeklyUseHistogramName,
                             usage_storage_->GetLastISOWeekSum(), 8);
}

void GeneralBrowserUsage::ReportProfileCount(size_t count) {
#if !BUILDFLAG(IS_ANDROID)
  p3a_utils::RecordToHistogramBucket(kProfileCountHistogramName,
                                     kProfileCountBuckets, count);
#endif  // !BUILDFLAG(IS_ANDROID)
}

void GeneralBrowserUsage::SetUpUpdateTimer() {
  report_timer_.Start(FROM_HERE, base::Time::Now() + kReportInterval, this,
                      &GeneralBrowserUsage::Update);
}

void GeneralBrowserUsage::Update() {
  ReportWeeklyUse();

  SetUpUpdateTimer();
}

}  // namespace misc_metrics
