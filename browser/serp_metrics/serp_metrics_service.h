/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_H_

#include <memory>

#include "components/keyed_service/core/keyed_service.h"

class PrefService;
class ProfileAttributesStorage;

namespace base {
class FilePath;
}  // namespace base

namespace serp_metrics {

class SerpMetrics;

// A keyed service that stores SERP metrics for a profile.
class SerpMetricsService : public KeyedService {
 public:
  SerpMetricsService(PrefService& local_state,
                     base::FilePath profile_path,
                     ProfileAttributesStorage& profile_attributes_storage);

  SerpMetricsService(const SerpMetricsService&) = delete;
  SerpMetricsService& operator=(const SerpMetricsService&) = delete;

  ~SerpMetricsService() override;

  // Returns `nullptr` if `SerpMetricsFeature` is disabled.
  SerpMetrics* Get();

 private:
  std::unique_ptr<SerpMetrics> serp_metrics_;
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_H_
