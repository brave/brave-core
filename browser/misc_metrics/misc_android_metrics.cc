/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/misc_android_metrics.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/misc_metrics/uptime_monitor.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/components/misc_metrics/privacy_hub_metrics.h"
#include "brave/components/misc_metrics/tab_metrics.h"

namespace misc_metrics {

MiscAndroidMetrics::MiscAndroidMetrics(
    ProcessMiscMetrics* misc_metrics,
    SearchEngineTracker* search_engine_tracker)
    : misc_metrics_(misc_metrics),
      search_engine_tracker_(search_engine_tracker) {}

MiscAndroidMetrics::~MiscAndroidMetrics() = default;

mojo::PendingRemote<mojom::MiscAndroidMetrics>
MiscAndroidMetrics::MakeRemote() {
  mojo::PendingRemote<mojom::MiscAndroidMetrics> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void MiscAndroidMetrics::RecordPrivacyHubView() {
  misc_metrics_->privacy_hub_metrics()->RecordView();
}

void MiscAndroidMetrics::RecordPrivacyHubEnabledStatus(bool is_enabled) {
  misc_metrics_->privacy_hub_metrics()->RecordEnabledStatus(is_enabled);
}

void MiscAndroidMetrics::RecordLocationBarChange(bool is_new_tab,
                                                 bool is_search_query) {
  if (is_search_query) {
    search_engine_tracker_->RecordLocationBarQuery();
  }
  misc_metrics_->tab_metrics()->RecordLocationBarChange(is_new_tab);
}

void MiscAndroidMetrics::RecordAppMenuNewTab() {
  misc_metrics_->tab_metrics()->RecordAppMenuNewTab();
}

void MiscAndroidMetrics::RecordTabSwitcherNewTab() {
  misc_metrics_->tab_metrics()->RecordTabSwitcherNewTab();
}

void MiscAndroidMetrics::RecordBrowserUsageDuration(base::TimeDelta duration) {
  misc_metrics_->uptime_monitor()->ReportUsageDuration(duration);
}

void MiscAndroidMetrics::RecordSetAsDefault(bool is_default) {
  UMA_HISTOGRAM_BOOLEAN(kBraveCoreIsDefaultHistogramName, is_default);
}

}  // namespace misc_metrics
