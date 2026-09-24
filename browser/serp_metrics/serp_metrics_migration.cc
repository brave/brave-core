/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_migration.h"

#include <utility>

#include "base/values.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

void MaybeMigrateSerpMetricsToProfileAttributes(PrefService& prefs,
                                                ProfileAttributesEntry& entry) {
  base::DictValue serp_metrics =
      prefs.GetDict(prefs::kDeprecatedSerpMetricsTimePeriodStorage).Clone();

  if (serp_metrics.empty()) {
    // No SERP metrics to migrate because they were already migrated or never
    // set.
    return;
  }
  prefs.ClearPref(prefs::kDeprecatedSerpMetricsTimePeriodStorage);

  // The migration runs only once during profile load (via OnProfileAdded),
  // before any navigation can record new metrics, so overwriting the SERP
  // metrics in ProfileAttributesEntry here is safe and lossless.
  entry.SetSerpMetrics(std::move(serp_metrics));
}

}  // namespace serp_metrics
