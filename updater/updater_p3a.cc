/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/updater/updater_p3a.h"

#include "base/check.h"
#include "base/metrics/histogram_macros.h"

namespace brave_updater {

constexpr char kFirstLaunchTimePref[] = "brave.updater_p3a.first_launch_time";
constexpr char kLastLaunchUsedOmaha4Pref[] =
    "brave.updater_p3a.last_launch_used_omaha4";
constexpr char kLastLaunchVersionPref[] =
    "brave.updater_p3a.last_launch_version";
constexpr char kLastReportedWeekPref[] = "brave.updater_p3a.last_reported_week";

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kFirstLaunchTimePref, {});
  registry->RegisterBooleanPref(kLastLaunchUsedOmaha4Pref, false);
  registry->RegisterStringPref(kLastLaunchVersionPref, "");
  registry->RegisterIntegerPref(kLastReportedWeekPref, -1);
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

  base::Time first_launch_time = prefs->GetTime(kFirstLaunchTimePref);
  if (first_launch_time.is_null()) {
    prefs->SetTime(kFirstLaunchTimePref, now);
    return;
  }

  base::TimeDelta time_since_first = now - first_launch_time;
  int current_week = time_since_first.InDays() / 7;

  int last_reported_week = prefs->GetInteger(kLastReportedWeekPref);
  if (last_reported_week >= current_week) {
    return;
  }

  bool updated_to_new_version = last_launch_version != current_version;

  UpdateStatus status;
  bool report_status = false;
  if (updated_to_new_version) {
    status = last_launch_used_omaha4 ? kUpdatedWithOmaha4 : kUpdatedWithLegacy;
    // Report updates immediately.
    report_status = true;
  } else {
    status = is_using_omaha4 ? kNoUpdateWithOmaha4 : kNoUpdateWithLegacy;
    // Only report "no update" when there was no update for at least one week.
    report_status = current_week > last_reported_week + 1;
  }

  if (report_status) {
    UMA_HISTOGRAM_ENUMERATION(kUpdateStatusHistogramName, status,
                              kUpdatedWithOmaha4 + 1);
    prefs->SetInteger(kLastReportedWeekPref, current_week);
  }
}

void SetLastLaunchVersionForTesting(std::string version, PrefService* prefs) {
  prefs->SetString(kLastLaunchVersionPref, version);
}

}  // namespace brave_updater
