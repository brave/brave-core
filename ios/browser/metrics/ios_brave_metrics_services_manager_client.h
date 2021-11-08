/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_METRICS_IOS_BRAVE_METRICS_SERVICES_MANAGER_CLIENT_H_
#define BRAVE_IOS_BROWSER_METRICS_IOS_BRAVE_METRICS_SERVICES_MANAGER_CLIENT_H_

#include <memory>
#include "ios/chrome/browser/metrics/ios_chrome_metrics_services_manager_client.h"

class PrefService;

namespace metrics {
class EnabledStateProvider;
class MetricsStateManager;
}

// Provides an //ios/brave-specific implementation of
// MetricsServicesManagerClient.
class IOSBraveMetricsServicesManagerClient
    : public IOSChromeMetricsServicesManagerClient {
 public:
  explicit IOSBraveMetricsServicesManagerClient(PrefService* local_state);
  IOSBraveMetricsServicesManagerClient(
      const IOSBraveMetricsServicesManagerClient&) = delete;
  IOSBraveMetricsServicesManagerClient& operator=(
      const IOSBraveMetricsServicesManagerClient&) = delete;
  ~IOSBraveMetricsServicesManagerClient() override;

 private:
  // metrics_services_manager::MetricsServicesManagerClient:
  std::unique_ptr<metrics::MetricsServiceClient> CreateMetricsServiceClient()
      override;
  bool IsMetricsReportingEnabled() override;
  bool IsMetricsConsentGiven() override;
  bool IsOffTheRecordSessionActive() override;
};

#endif  // BRAVE_IOS_BROWSER_METRICS_IOS_BRAVE_METRICS_SERVICES_MANAGER_CLIENT_H_
