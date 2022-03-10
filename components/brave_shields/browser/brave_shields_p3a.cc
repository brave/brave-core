/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_p3a.h"

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/p3a/brave_p3a_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_shields {
namespace {

void RecordShieldsLevelSetting(const char* histogram_name,
                               ControlType setting) {
  int hg_value;
  switch (setting) {
    case ControlType::ALLOW:
      hg_value = 0;
      break;
    case ControlType::BLOCK_THIRD_PARTY:
    case ControlType::DEFAULT:
      hg_value = 1;
      break;
    case ControlType::BLOCK:
      hg_value = 2;
      break;
    default:
      return;
  }
  base::UmaHistogramExactLinear(histogram_name, hg_value, 3);
}

}  // namespace

void MaybeRecordShieldsUsageP3A(ShieldsIconUsage usage,
                                PrefService* local_state) {
  ::brave::RecordValueIfGreater<ShieldsIconUsage>(
      usage, kUsageStatusHistogramName, kUsagePrefName, local_state);
}

void RecordShieldsAdsSetting(ControlType setting) {
  RecordShieldsLevelSetting(kAdsSettingHistogramName, setting);
}

void MaybeRecordInitialShieldsSettings(PrefService* profile_prefs,
                                       HostContentSettingsMap* map) {
  if (profile_prefs->GetBoolean(kFirstReportedPrefName)) {
    return;
  }
  VLOG(1) << "ShieldsP3A: Initial report of profile";
  RecordShieldsAdsSetting(GetCosmeticFilteringControlType(map, GURL()));
  RecordShieldsFingerprintSetting(GetFingerprintingControlType(map, GURL()));
  profile_prefs->SetBoolean(kFirstReportedPrefName, true);
}

void RecordShieldsFingerprintSetting(ControlType setting) {
  RecordShieldsLevelSetting(kFingerprintSettingHistogramName, setting);
}

void RegisterShieldsP3ALocalPrefs(PrefRegistrySimple* local_state) {
  local_state->RegisterIntegerPref(kUsagePrefName, -1);
}

void RegisterShieldsP3AProfilePrefs(PrefRegistrySimple* profile_state) {
  profile_state->RegisterBooleanPref(kFirstReportedPrefName, false);
}

}  // namespace brave_shields
