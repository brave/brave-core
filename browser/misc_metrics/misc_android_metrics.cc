/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/misc_android_metrics.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/components/misc_metrics/privacy_hub_metrics.h"

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

void MiscAndroidMetrics::RecordLocationBarQuery() {
  search_engine_tracker_->RecordLocationBarQuery();
}

}  // namespace misc_metrics
