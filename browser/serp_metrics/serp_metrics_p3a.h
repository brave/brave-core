/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_P3A_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_P3A_H_

#include <string>
#include <string_view>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "brave/components/p3a/metric_log_type.h"

class PrefRegistrySimple;
class PrefService;
class ProfileAttributesStorage;
class ProfileManager;

namespace p3a {
class P3AService;
}  // namespace p3a

namespace serp_metrics {

// Histogram names for the daily/express P3A SERP metrics.
inline constexpr char kBraveSerpHistogramName[] = "Brave.Search.SERPBrave";
inline constexpr char kGoogleSerpHistogramName[] = "Brave.Search.SERPGoogle";
inline constexpr char kOtherSerpHistogramName[] = "Brave.Search.SERPOther";
inline constexpr char kStaleSerpHistogramName[] = "Brave.Search.SERPStale";

// Duplicates the SERP metrics reported in the usage ping, via P3A.
class SerpMetricsP3A {
 public:
  explicit SerpMetricsP3A(PrefService& local_state);
  ~SerpMetricsP3A();

  SerpMetricsP3A(const SerpMetricsP3A&) = delete;
  SerpMetricsP3A& operator=(const SerpMetricsP3A&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Registers P3A rotation and metric-cycled callbacks. Must be called on the
  // UI thread after `P3AService` is initialized.
  void Init(p3a::P3AService* p3a_service, ProfileManager* profile_manager);

  // Invoked via P3AService rotation callback.
  void OnRotation(p3a::MetricLogType log_type);

  // Invoked via P3AService metric-cycled callback.
  void OnMetricCycled(const std::string& histogram_name);

 private:
  void ReportMetrics();

  base::Time GetLastReportedTime(std::string_view dict_key) const;

  const raw_ref<PrefService> local_state_;
  raw_ptr<ProfileAttributesStorage> profile_attributes_storage_ = nullptr;
  base::CallbackListSubscription rotation_subscription_;
  base::CallbackListSubscription metric_cycled_subscription_;
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_P3A_H_
