/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/metrics/ios_brave_metrics_services_manager_client.h"

#include "components/metrics/metrics_service_client.h"
#include "components/prefs/pref_service.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSBraveMetricsServicesManagerClient::IOSBraveMetricsServicesManagerClient(
    PrefService* local_state)
    : IOSChromeMetricsServicesManagerClient(local_state) {}

IOSBraveMetricsServicesManagerClient::
    ~IOSBraveMetricsServicesManagerClient() = default;

std::unique_ptr<metrics::MetricsServiceClient>
IOSBraveMetricsServicesManagerClient::CreateMetricsServiceClient() {
  return nullptr;
}

bool IOSBraveMetricsServicesManagerClient::IsMetricsReportingEnabled() {
  return false;
}

bool IOSBraveMetricsServicesManagerClient::IsMetricsConsentGiven() {
  return false;
}

bool IOSBraveMetricsServicesManagerClient::IsOffTheRecordSessionActive() {
  return false;
}
