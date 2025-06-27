/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UPDATER_UPDATER_P3A_H_
#define BRAVE_UPDATER_UPDATER_P3A_H_

#include <string>

#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

// We're migrating Brave's auto-update mechanism to Omaha 4. As we do this, we
// want to compare the success rates of the Omaha 4 and legacy implementations.
// The functions in this file achieve this by reporting UMA events when the
// browser was (or was not) updated.

namespace brave_updater {

// This function is called when the browser launches. It remembers the browser
// version in a pref. When the version is different from the last launch, it
// reports to UMA that the browser was updated. When no such update took place
// in one week, it reports this to UMA as well. The reports include whether
// Omaha 4 or the legacy updater were used. This lets us compare the success
// rates of the two implementations.
void ReportLaunch(base::Time now,
                  std::string current_version,
                  bool is_using_omaha4,
                  PrefService* prefs);

// Register the prefs for use by ReportLaunch(...) above.
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

void SetLastLaunchVersionForTesting(std::string version, PrefService* prefs);

inline constexpr char kUpdateStatusHistogramName[] = "Brave.Update.Status";

enum UpdateStatus {
  kNoUpdateWithLegacy,
  kNoUpdateWithOmaha4,
  kUpdatedWithLegacy,
  kUpdatedWithOmaha4
};

}  // namespace brave_updater

#endif  // BRAVE_UPDATER_UPDATER_P3A_H_
