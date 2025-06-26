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

namespace brave_updater {

inline constexpr char kUpdateStatusHistogramName[] = "Brave.Update.Status";

enum UpdateStatus {
  kNoUpdateWithLegacy,
  kNoUpdateWithOmaha4,
  kUpdatedWithLegacy,
  kUpdatedWithOmaha4
};

void RegisterLocalState(PrefRegistrySimple* registry);
void ReportLaunch(base::Time now,
                  std::string current_version,
                  bool is_using_omaha4,
                  PrefService* prefs);
void SetLastLaunchVersionForTesting(std::string version, PrefService* prefs);

}  // namespace brave_updater

#endif  // BRAVE_UPDATER_UPDATER_P3A_H_
