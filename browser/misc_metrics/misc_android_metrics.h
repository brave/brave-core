/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_H_

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/common/misc_metrics.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class SearchEngineTracker;

namespace misc_metrics {

inline constexpr char kBraveCoreIsDefaultHistogramName[] =
    "Brave.Core.IsDefault";

class ProcessMiscMetrics;

class MiscAndroidMetrics : public mojom::MiscAndroidMetrics {
 public:
  MiscAndroidMetrics(ProcessMiscMetrics* misc_metrics,
                     SearchEngineTracker* search_engine_tracker);
  ~MiscAndroidMetrics() override;

  MiscAndroidMetrics(const MiscAndroidMetrics&) = delete;
  MiscAndroidMetrics& operator=(const MiscAndroidMetrics&) = delete;

  mojo::PendingRemote<mojom::MiscAndroidMetrics> MakeRemote();

  // mojom::MiscAndroidMetrics:
  void RecordPrivacyHubView() override;
  void RecordPrivacyHubEnabledStatus(bool is_enabled) override;
  void RecordBrowserUsageDuration(base::TimeDelta duration) override;
  void RecordLocationBarChange(bool is_new_tab, bool is_search_query) override;
  void RecordAppMenuNewTab() override;
  void RecordTabSwitcherNewTab() override;
  void RecordSetAsDefault(bool is_default) override;

 private:
  raw_ptr<ProcessMiscMetrics> misc_metrics_;
  raw_ptr<SearchEngineTracker> search_engine_tracker_;

  mojo::ReceiverSet<mojom::MiscAndroidMetrics> receivers_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_H_
