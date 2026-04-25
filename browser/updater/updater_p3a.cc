/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/updater_p3a.h"

#include "base/check.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"

namespace brave_updater {

// This pref remembers whether Brave used Omaha 4 or the legacy updater in the
// last browser launch:
constexpr char kLastLaunchUsedOmaha4Pref[] =
    "brave.updater_p3a.last_launch_used_omaha4";

// This pref remembers the version of the browser that was last launched:
constexpr char kLastLaunchVersionPref[] =
    "brave.updater_p3a.last_launch_version";

// This pref remembers when the browser was last updated:
constexpr char kLastUpdateTimePref[] = "brave.updater_p3a.last_update_time";

// This pref remembers whether the last update was done with Omaha 4 or the
// legacy updater:
constexpr char kLastUpdateUsedOmaha4Pref[] =
    "brave.updater_p3a.last_update_used_omaha4";

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kLastLaunchVersionPref, "");
  registry->RegisterBooleanPref(kLastLaunchUsedOmaha4Pref, false);
  registry->RegisterTimePref(kLastUpdateTimePref, {});
  registry->RegisterBooleanPref(kLastUpdateUsedOmaha4Pref, false);
}

void ReportLaunch(std::string current_version,
                  bool is_using_omaha4,
                  PrefService* prefs) {
  CHECK(prefs);
  auto now = base::Time::Now();
  std::string last_launch_version = prefs->GetString(kLastLaunchVersionPref);
  prefs->SetString(kLastLaunchVersionPref, current_version);

  bool last_launch_used_omaha4 = prefs->GetBoolean(kLastLaunchUsedOmaha4Pref);
  prefs->SetBoolean(kLastLaunchUsedOmaha4Pref, is_using_omaha4);

  if (last_launch_version.empty()) {
    // This is the first launch.
    return;
  }

  base::Time last_update_time;
  bool last_update_used_omaha4;

  if (last_launch_version != current_version) {
    last_update_time = now;
    prefs->SetTime(kLastUpdateTimePref, now);

    last_update_used_omaha4 = last_launch_used_omaha4;
    // We remember the Omaha 4 state at the time of update because it is
    // controlled by a feature flag and can change between launches.
    prefs->SetBoolean(kLastUpdateUsedOmaha4Pref, last_launch_used_omaha4);
  } else {
    last_update_time = prefs->GetTime(kLastUpdateTimePref);
    last_update_used_omaha4 = prefs->GetBoolean(kLastUpdateUsedOmaha4Pref);
  }

  bool updated_in_past_7_days =
      !last_update_time.is_null() && (now - last_update_time).InDays() < 7;

  UpdateStatus status;
  using enum UpdateStatus;
  if (updated_in_past_7_days) {
    status = last_update_used_omaha4 ? kUpdatedWithOmaha4 : kUpdatedWithLegacy;
  } else {
    status = is_using_omaha4 ? kNoUpdateWithOmaha4 : kNoUpdateWithLegacy;
  }

  UMA_HISTOGRAM_ENUMERATION(kUpdateStatusHistogramName, status);
}

}  // namespace brave_updater
