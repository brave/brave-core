// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_SHIELDS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_SHIELDS_P3A_H_

#include "brave/components/brave_shields/content/browser/brave_shields_util.h"

class PrefRegistrySimple;
class PrefService;
class HostContentSettingsMap;

namespace brave_shields {

inline constexpr char kUsagePrefName[] = "brave_shields.p3a_usage";
inline constexpr char kFirstReportedPrefName[] =
    "brave_shields.p3a_first_reported_v2";  // DEPRECATED
inline constexpr char kFirstReportedRevisionPrefName[] =
    "brave_shields.p3a_first_reported_revision";

inline constexpr char kAdsStrictCountPrefName[] =
    "brave_shields.p3a_ads_strict_domain_count";
inline constexpr char kAdsStandardCountPrefName[] =
    "brave_shields.p3a_ads_standard_domain_count";
inline constexpr char kAdsAllowCountPrefName[] =
    "brave_shields.p3a_ads_allow_domain_count";
inline constexpr char kFPStrictCountPrefName[] =
    "brave_shields.p3a_fp_strict_domain_count";
inline constexpr char kFPStandardCountPrefName[] =
    "brave_shields.p3a_fp_standard_domain_count";
inline constexpr char kFPAllowCountPrefName[] =
    "brave_shields.p3a_fp_allow_domain_count";

inline constexpr char kAdsSettingHistogramName[] =
    "Brave.Shields.AdBlockSetting";
inline constexpr char kFingerprintSettingHistogramName[] =
    "Brave.Shields.FingerprintBlockSetting";
inline constexpr char kUsageStatusHistogramName[] = "Brave.Shields.UsageStatus";
inline constexpr char kDomainAdsSettingsAboveHistogramName[] =
    "Brave.Shields.DomainAdsSettingsAboveGlobal";
inline constexpr char kDomainAdsSettingsBelowHistogramName[] =
    "Brave.Shields.DomainAdsSettingsBelowGlobal";
inline constexpr char kDomainFPSettingsAboveHistogramName[] =
    "Brave.Shields.DomainFingerprintSettingsAboveGlobal";
inline constexpr char kDomainFPSettingsBelowHistogramName[] =
    "Brave.Shields.DomainFingerprintSettingsBelowGlobal";
inline constexpr char kForgetFirstPartyHistogramName[] =
    "Brave.Shields.ForgetFirstParty";
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

// Records to global ads setting histogram: Brave.Shields.AdBlockSetting
void RecordShieldsAdsSetting(ControlType setting);

// Records to global FP setting histogram: Brave.Shields.FingerprintBlockSetting
void RecordShieldsFingerprintSetting(ControlType setting);

// To be called when the global setting changes.
// Will update domain setting count histograms.
void RecordShieldsDomainSettingCounts(PrefService* profile_prefs,
                                      bool is_fingerprint,
                                      ControlType global_setting);

// To be called when a domain setting changes.
// Will update internal pref counts and update domain setting count histograms.
void RecordShieldsDomainSettingCountsWithChange(PrefService* profile_prefs,
                                                bool is_fingerprint,
                                                ControlType global_setting,
                                                ControlType* prev_setting,
                                                ControlType new_setting);

// Records global "forget me when I close this site" setting,
// and any per-site exceptions.
void RecordForgetFirstPartySetting(HostContentSettingsMap* map);

void RegisterShieldsP3ALocalPrefs(PrefRegistrySimple* local_state);

void RegisterShieldsP3AProfilePrefs(PrefRegistrySimple* registry);
void RegisterShieldsP3AProfilePrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteProfilePrefs(PrefService* profile_prefs);

// To be called at initialization. Will count all domain settings and
// record to all histograms, if executed for the first time.
void MaybeRecordInitialShieldsSettings(PrefService* profile_prefs,
                                       HostContentSettingsMap* map);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_SHIELDS_P3A_H_
