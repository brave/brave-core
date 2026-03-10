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

// Added 2026-03
void MaybeMigrateSerpMetricsToProfileAttributes(PrefService& prefs,
                                                ProfileAttributesEntry& entry) {
  base::DictValue serp_metrics =
      prefs.GetDict(prefs::kDeprecatedSerpMetricsTimePeriodStorage).Clone();

  // No SERP metrics to migrate because they were already migrated or never set.
  if (serp_metrics.empty()) {
    return;
  }
  prefs.ClearPref(prefs::kDeprecatedSerpMetricsTimePeriodStorage);
  entry.SetSerpMetrics(std::move(serp_metrics));
}

}  // namespace serp_metrics
