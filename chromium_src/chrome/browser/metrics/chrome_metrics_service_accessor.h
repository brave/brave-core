/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_METRICS_CHROME_METRICS_SERVICE_ACCESSOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_METRICS_CHROME_METRICS_SERVICE_ACCESSOR_H_

namespace metrics {
bool ShouldShowCrashReportPermissionAskDialog();
}

bool IsMetricsReportingOptInAndroid();

#define MetricsReportingStateTest                                  \
  MetricsReportingStateTest;                                       \
  friend bool metrics::ShouldShowCrashReportPermissionAskDialog(); \
  friend bool IsMetricsReportingOptInAndroid();                    \
  friend class BraveWelcomeUI
#include "src/chrome/browser/metrics/chrome_metrics_service_accessor.h"  // IWYU pragma: export
#undef MetricsReportingStateTest

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FIRST_RUN_FIRST_RUN_H_
