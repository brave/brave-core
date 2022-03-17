/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/logging.h"
#include "base/time/time.h"
#include "brave/common/pref_names.h"
#include "brave/components/p3a/brave_p3a_version_util.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

constexpr int kReleaseFrequencyDays = 28;

void MaybeUpdateLastKnownVersion(PrefService* local_state,
                                 std::string curr_version) {
  std::string last_known_version =
      local_state->GetString(kP3ALastKnownInstalledVersion);
  if (last_known_version != curr_version) {
    VLOG(1) << "BraveP3AVersionUtil: Updating lask known installed version to: "
            << curr_version;
    local_state->SetString(kP3ALastKnownInstalledVersion, curr_version);
    local_state->SetTime(kP3ACurrentVersionInstallTime, base::Time::Now());
  }
}

}  // namespace

void RegisterP3AVersionUtilPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kP3ALastKnownInstalledVersion, std::string());
  registry->RegisterTimePref(kP3ACurrentVersionInstallTime, base::Time());
}

bool IsBrowserAtLatestVersion(PrefService* local_state) {
  std::string curr_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();
  MaybeUpdateLastKnownVersion(local_state, curr_version);

  bool is_usage_ping_enabled = local_state->GetBoolean(kStatsReportingEnabled);
  if (is_usage_ping_enabled) {
    std::string latest_version = local_state->GetString(kLatestBrowserVersion);
    if (!latest_version.empty()) {
      return curr_version == latest_version;
    }
  }

  base::Time curr_version_install_time =
      local_state->GetTime(kP3ACurrentVersionInstallTime);

  base::TimeDelta installed_duration =
      base::Time::Now() - curr_version_install_time;

  return installed_duration.InDays() <= kReleaseFrequencyDays;
}

}  // namespace brave
