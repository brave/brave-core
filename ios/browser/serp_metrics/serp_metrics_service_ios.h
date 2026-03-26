/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_IOS_H_
#define BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_IOS_H_

#include <memory>

#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace serp_metrics {

class SerpMetrics;

// A keyed service that stores SERP metrics for an iOS profile.
class SerpMetricsServiceIOS : public KeyedService {
 public:
  SerpMetricsServiceIOS(PrefService& local_state, PrefService& profile_prefs);

  SerpMetricsServiceIOS(const SerpMetricsServiceIOS&) = delete;
  SerpMetricsServiceIOS& operator=(const SerpMetricsServiceIOS&) = delete;

  ~SerpMetricsServiceIOS() override;

  // KeyedService:
  void Shutdown() override;

  // Returns nullptr if `kSerpMetricsFeature` is disabled.
  SerpMetrics* Get();

 private:
  std::unique_ptr<SerpMetrics> serp_metrics_;
};

}  // namespace serp_metrics

#endif  // BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_IOS_H_
