/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_METRICS_CHROME_METRICS_SERVICE_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_METRICS_CHROME_METRICS_SERVICE_CLIENT_H_

#define ChromeMetricsServiceClient ChromeMetricsServiceClient_ChromiumImpl
#define RegisterMetricsServiceProviders virtual RegisterMetricsServiceProviders
#include <chrome/browser/metrics/chrome_metrics_service_client.h>  // IWYU pragma: export
#undef RegisterMetricsServiceProviders
#undef ChromeMetricsServiceClient

// The purpose of this override is to supress the Chromium metrics providers
// registration. The providers were used only to report UMA/UKM metrics.
class ChromeMetricsServiceClient
    : public ChromeMetricsServiceClient_ChromiumImpl {
 public:
  ChromeMetricsServiceClient(
      metrics::MetricsStateManager* state_manager,
      variations::SyntheticTrialRegistry* synthetic_trial_registry);

 private:
  void RegisterMetricsServiceProviders() override;
  void RegisterUKMProviders() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_METRICS_CHROME_METRICS_SERVICE_CLIENT_H_
