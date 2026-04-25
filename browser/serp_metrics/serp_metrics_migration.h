/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_MIGRATION_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_MIGRATION_H_

class PrefService;
class ProfileAttributesEntry;

namespace serp_metrics {

// Migrates the deprecated SERP metrics preference to profile attributes and
// clears the preference. No-op if the preference is already empty, so it is
// safe to be called multiple times.
void MaybeMigrateSerpMetricsToProfileAttributes(PrefService& prefs,
                                                ProfileAttributesEntry& entry);

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_MIGRATION_H_
