/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_SERP_METRICS_PREF_NAMES_H_

#include <string_view>

namespace serp_metrics::prefs {

// Deprecated in 2026-03.
// When removing the preference, also remove the
// `MaybeMigrateSerpMetricsToProfileAttributes` function.
inline constexpr std::string_view kDeprecatedSerpMetricsTimePeriodStorage =
    "brave.stats.serp_metrics";

}  // namespace serp_metrics::prefs

#endif  // BRAVE_COMPONENTS_SERP_METRICS_PREF_NAMES_H_
