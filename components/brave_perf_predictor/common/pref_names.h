/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_COMMON_PREF_NAMES_H_

namespace brave_perf_predictor {

namespace prefs {

inline constexpr char kBandwidthSavedBytes[] =
    "brave.stats.bandwidth_saved_bytes";
inline constexpr char kBandwidthSavedDailyBytes[] =
    "brave.stats.daily_saving_predictions_bytes";

}  // namespace prefs

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_COMMON_PREF_NAMES_H_
