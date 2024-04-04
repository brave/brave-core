/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_METRICS_SWITCHES_H_
#define BRAVE_BROWSER_METRICS_SWITCHES_H_

namespace metrics::switches {

// force crash reporting to opt-in
inline constexpr char kForceMetricsOptInEnabled[] =
    "force-metrics-opt-in-enabled";

// force crash reporting to opt-out
inline constexpr char kForceMetricsOptInDisabled[] =
    "force-metrics-opt-in-disabled";

}  // namespace metrics::switches

#endif  // BRAVE_BROWSER_METRICS_SWITCHES_H_
