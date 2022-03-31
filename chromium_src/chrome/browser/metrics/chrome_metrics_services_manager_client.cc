/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/metrics/chrome_metrics_services_manager_client.h"

#define IsMetricsReportingEnabled IsMetricsReportingEnabled_ChromiumImpl
#include "src/chrome/browser/metrics/chrome_metrics_services_manager_client.cc"
#undef IsMetricsReportingEnabled

bool ChromeMetricsServicesManagerClient::IsMetricsReportingEnabled() {
  return false;
}
