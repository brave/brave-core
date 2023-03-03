/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/general_browser_usage.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/time_period_storage/iso_weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

const base::TimeDelta kReportInterval = base::Days(1);
const char kWeeklyUseHistogramName[] = "Brave.Core.WeeklyUsage";

GeneralBrowserUsage::GeneralBrowserUsage(PrefService* local_state) {
  usage_storage_ = std::make_unique<ISOWeeklyStorage>(
      local_state, kMiscMetricsBrowserUsageList);
  report_timer_.Start(FROM_HERE, kReportInterval, this,
                      &GeneralBrowserUsage::Update);
  Update();
}

GeneralBrowserUsage::~GeneralBrowserUsage() = default;

void GeneralBrowserUsage::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsBrowserUsageList);
}

void GeneralBrowserUsage::Update() {
  usage_storage_->ReplaceTodaysValueIfGreater(1);
  UMA_HISTOGRAM_EXACT_LINEAR(kWeeklyUseHistogramName,
                             usage_storage_->GetLastISOWeekSum(), 8);
}

}  // namespace misc_metrics
