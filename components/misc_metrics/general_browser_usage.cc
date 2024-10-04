/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/general_browser_usage.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/iso_weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Minutes(10);

#if !BUILDFLAG(IS_ANDROID)
constexpr int kProfileCountBuckets[] = {0, 1, 2, 3, 5};
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace

GeneralBrowserUsage::GeneralBrowserUsage(
    PrefService* local_state,
    std::optional<std::string> day_zero_experiment_variant,
    bool is_first_run,
    base::Time first_run_time)
    : local_state_(local_state), first_run_time_(first_run_time) {
  usage_storage_ = std::make_unique<ISOWeeklyStorage>(
      local_state, kMiscMetricsBrowserUsageList);

  if (is_first_run) {
    if (day_zero_experiment_variant) {
      local_state->SetString(kMiscMetricsDayZeroVariantAtInstall,
                             *day_zero_experiment_variant);
    }
  }
  if (first_run_time.is_null()) {
    first_run_time_ = base::Time::Now();
  }

  Update();
}

GeneralBrowserUsage::~GeneralBrowserUsage() = default;

void GeneralBrowserUsage::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsBrowserUsageList);
  registry->RegisterStringPref(kMiscMetricsDayZeroVariantAtInstall, {});
}

void GeneralBrowserUsage::ReportWeeklyUse() {
  usage_storage_->ReplaceTodaysValueIfGreater(1);
  UMA_HISTOGRAM_EXACT_LINEAR(kWeeklyUseHistogramName,
                             usage_storage_->GetLastISOWeekSum(), 8);

  // TODO(djandries): remove the following report when Nebula experiment is over
  UMA_HISTOGRAM_EXACT_LINEAR(kWeeklyUseNebulaHistogramName,
                             usage_storage_->GetLastISOWeekSum(), 8);
}

#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_WIN)
void GeneralBrowserUsage::ReportInstallTime() {
  int days_since_install = (base::Time::Now() - first_run_time_).InDays();
  if (days_since_install < 0 || days_since_install > 30) {
    return;
  }
  std::string day_zero_variant =
      local_state_->GetString(kMiscMetricsDayZeroVariantAtInstall);
  if (day_zero_variant.empty()) {
    return;
  }
  std::string histogram_name = base::StrCat(
      {kDayZeroInstallTimePrefix, base::ToUpperASCII(day_zero_variant),
       kDayZeroInstallTimeSuffix});
  base::UmaHistogramExactLinear(histogram_name, days_since_install, 31);
}
#endif  // BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_WIN)

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
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_WIN)
  ReportInstallTime();
#endif  // BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_WIN)

  SetUpUpdateTimer();
}

}  // namespace misc_metrics
