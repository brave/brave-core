/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_PREF_NAMES_H_

namespace metrics::prefs {

// Local State prefs.
inline constexpr char kObliviousHttpKeyConfig[] =
    "brave.metrics.search_query.ohttp.key_config.public_key";
inline constexpr char kObliviousHttpKeyConfigExpiresAt[] =
    "brave.metrics.search_query.ohttp.key_config.expires_at";

// Profile prefs.
inline constexpr char kLastReportedAt[] =
    "brave.metrics.search_query.last_reported_at";

}  // namespace metrics::prefs

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_PREF_NAMES_H_
