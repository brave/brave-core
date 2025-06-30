/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/updater/updater_p3a.h"

#include "base/check.h"
#include "base/metrics/histogram_macros.h"

namespace brave_updater {

constexpr char kLastLaunchUsedOmaha4Pref[] =
    "brave.updater_p3a.last_launch_used_omaha4";
constexpr char kLastLaunchVersionPref[] =
    "brave.updater_p3a.last_launch_version";
constexpr char kLastUpdateTimePref[] = "brave.updater_p3a.last_update_time";
constexpr char kLastUpdateUsedOmaha4Pref[] =
    "brave.updater_p3a.last_update_used_omaha4";

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kLastLaunchVersionPref, "");
  registry->RegisterBooleanPref(kLastLaunchUsedOmaha4Pref, false);
  registry->RegisterTimePref(kLastUpdateTimePref, {});
  registry->RegisterBooleanPref(kLastUpdateUsedOmaha4Pref, false);
}

void ReportLaunch(base::Time now,
                  std::string current_version,
                  bool is_using_omaha4,
                  PrefService* prefs) {
  DCHECK(prefs);
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
    prefs->SetBoolean(kLastUpdateUsedOmaha4Pref, last_launch_used_omaha4);
  } else {
    last_update_time = prefs->GetTime(kLastUpdateTimePref);
    last_update_used_omaha4 = prefs->GetBoolean(kLastUpdateUsedOmaha4Pref);
  }

  bool updated_this_week =
      !last_update_time.is_null() && (now - last_update_time).InDays() < 7;

  UpdateStatus status;
  using enum UpdateStatus;
  if (updated_this_week) {
    status = last_update_used_omaha4 ? kUpdatedWithOmaha4 : kUpdatedWithLegacy;
  } else {
    status = is_using_omaha4 ? kNoUpdateWithOmaha4 : kNoUpdateWithLegacy;
  }

  UMA_HISTOGRAM_ENUMERATION(kUpdateStatusHistogramName, status);
}

void SetLastLaunchVersionForTesting(std::string version, PrefService* prefs) {
  prefs->SetString(kLastLaunchVersionPref, version);
}

}  // namespace brave_updater
