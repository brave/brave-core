/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_P3A_H_

#include "brave/components/brave_shields/browser/brave_shields_util.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_shields {

constexpr char kUsagePrefName[] = "brave_shields.p3a_usage";
constexpr char kAdsDefaultReportedPrefName[] =
    "brave_shields.ads_default_reported";
constexpr char kFingerprintDefaultReportedPrefName[] =
    "brave_shields.fingerprint_default_reported";
constexpr char kAdsSettingHistogramName[] = "Brave.Shields.AdBlockSetting";
constexpr char kFingerprintSettingHistogramName[] =
    "Brave.Shields.FingerprintBlockSetting";
constexpr char kUsageStatusHistogramName[] = "Brave.Shields.UsageStatus";
// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
enum ShieldsIconUsage {
  kNeverClicked,
  kClicked,
  kShutOffShields,
  kChangedPerSiteShields,
  kSize,
};

// We save latest value to local state and compare new values with it.
// The idea is to write to a histogram only the highest value (e.g. we are
// not interested in |kClicked| event if the user already turned off shields.
// Sine P3A sends only latest written values, these is enough for our current
// goals.
void MaybeRecordShieldsUsageP3A(ShieldsIconUsage usage,
                                PrefService* local_state);

void MaybeRecordDefaultShieldsAdsSetting(PrefService* local_state);

void MaybeRecordDefaultShieldsFingerprintSetting(PrefService* local_state);

void RecordShieldsAdsSetting(ControlType setting);

void RecordShieldsFingerprintSetting(ControlType setting);

void RegisterShieldsP3APrefs(PrefRegistrySimple* local_state);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_P3A_H_
