/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_p3a.h"

#include <algorithm>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_utils.h"
#include "brave/components/p3a/brave_p3a_utils.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_shields {
namespace {

constexpr int kDomainCountBuckets[] = {0, 5, 10, 20, 30};

constexpr ControlType kFPSettingOrder[] = {
    ControlType::ALLOW, ControlType::DEFAULT, ControlType::BLOCK};

constexpr ControlType kAdsSettingOrder[] = {
    ControlType::ALLOW, ControlType::BLOCK_THIRD_PARTY, ControlType::BLOCK};

constexpr int kSettingCount = 3;

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

const char* GetDomainSettingCountPrefName(bool is_fingerprint,
                                          ControlType setting) {
  if (is_fingerprint) {
    switch (setting) {
      case ControlType::ALLOW:
        return kFPAllowCountPrefName;
      case ControlType::DEFAULT:
        return kFPStandardCountPrefName;
      case ControlType::BLOCK:
        return kFPStrictCountPrefName;
      default:
        return nullptr;
    }
  } else {
    switch (setting) {
      case ControlType::ALLOW:
        return kAdsAllowCountPrefName;
      case ControlType::BLOCK_THIRD_PARTY:
        return kAdsStandardCountPrefName;
      case ControlType::BLOCK:
        return kAdsStrictCountPrefName;
      default:
        return nullptr;
    }
  }
}

int GetDomainSettingCount(PrefService* profile_prefs,
                          bool is_fingerprint,
                          ControlType setting) {
  const char* pref_name =
      GetDomainSettingCountPrefName(is_fingerprint, setting);
  if (pref_name == nullptr) {
    return 0;
  }
  return profile_prefs->GetInteger(pref_name);
}

void UpdateDomainSettingCount(PrefService* profile_prefs,
                              bool is_fingerprint,
                              ControlType setting,
                              int change) {
  const char* pref_name =
      GetDomainSettingCountPrefName(is_fingerprint, setting);
  if (pref_name == nullptr) {
    return;
  }
  int new_count = profile_prefs->GetInteger(pref_name) + change;
  profile_prefs->SetInteger(pref_name, new_count);
}

int DomainCountRelativeToGlobalSetting(PrefService* profile_prefs,
                                       bool is_fingerprint,
                                       ControlType global_setting,
                                       bool count_above) {
  const ControlType* setting_order =
      is_fingerprint ? kFPSettingOrder : kAdsSettingOrder;
  const ControlType* global_setting_order_it =
      std::find(setting_order, setting_order + kSettingCount, global_setting);
  if (global_setting_order_it == setting_order + kSettingCount) {
    return 0;
  }
  int total = 0;
  int sum_index_start =
      global_setting_order_it - setting_order + (count_above ? 1 : -1);
  for (int i = sum_index_start; count_above ? i < kSettingCount : i >= 0;
       count_above ? i++ : i--) {
    total +=
        GetDomainSettingCount(profile_prefs, is_fingerprint, setting_order[i]);
  }
  return total;
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

void RecordShieldsFingerprintSetting(ControlType setting) {
  RecordShieldsLevelSetting(kFingerprintSettingHistogramName, setting);
}

void RecordShieldsDomainSettingCounts(PrefService* profile_prefs,
                                      bool is_fingerprint,
                                      ControlType global_setting) {
  if (profile_prefs == nullptr) {
    return;
  }
  const char* above_hg_name = is_fingerprint
                                  ? kDomainFPSettingsAboveHistogramName
                                  : kDomainAdsSettingsAboveHistogramName;
  const char* below_hg_name = is_fingerprint
                                  ? kDomainFPSettingsBelowHistogramName
                                  : kDomainAdsSettingsBelowHistogramName;
  int above_total = DomainCountRelativeToGlobalSetting(
      profile_prefs, is_fingerprint, global_setting, true);
  int below_total = DomainCountRelativeToGlobalSetting(
      profile_prefs, is_fingerprint, global_setting, false);
  VLOG(1) << "BraveShieldsP3A: Recording counts: is_fp=" << is_fingerprint
          << " above=" << above_total << " below=" << below_total;
  p3a_utils::RecordToHistogramBucket(above_hg_name, kDomainCountBuckets,
                                     above_total);
  p3a_utils::RecordToHistogramBucket(below_hg_name, kDomainCountBuckets,
                                     below_total);
}

void RecordShieldsDomainSettingCountsWithChange(PrefService* profile_prefs,
                                                bool is_fingerprint,
                                                ControlType global_setting,
                                                ControlType* prev_setting,
                                                ControlType new_setting) {
  if (profile_prefs == nullptr) {
    return;
  }
  if (prev_setting != nullptr) {
    UpdateDomainSettingCount(profile_prefs, is_fingerprint, *prev_setting, -1);
    VLOG(1) << "BraveShieldsP3A: Decreasing prev setting count: prev_setting="
            << *prev_setting << " is_fp=" << is_fingerprint << " count="
            << GetDomainSettingCount(profile_prefs, is_fingerprint,
                                     *prev_setting);
  }
  UpdateDomainSettingCount(profile_prefs, is_fingerprint, new_setting, 1);
  VLOG(1) << "BraveShieldsP3A: Increasing new setting count: new_setting="
          << new_setting << " is_fp=" << is_fingerprint << " count="
          << GetDomainSettingCount(profile_prefs, is_fingerprint, new_setting);
  RecordShieldsDomainSettingCounts(profile_prefs, is_fingerprint,
                                   global_setting);
}

void MaybeRecordInitialShieldsSettings(PrefService* profile_prefs,
                                       HostContentSettingsMap* map) {
  if (profile_prefs->GetBoolean(kFirstReportedPrefName)) {
    return;
  }
  VLOG(1) << "BraveShieldsP3A: Starting initial report for profile";

  ControlType global_ads_setting = GetCosmeticFilteringControlType(map, GURL());
  ControlType global_fp_setting = GetFingerprintingControlType(map, GURL());
  RecordShieldsAdsSetting(global_ads_setting);
  RecordShieldsFingerprintSetting(global_fp_setting);

  ShieldsSettingCounts fp_counts = GetFPSettingCount(map);
  ShieldsSettingCounts ads_counts = GetAdsSettingCount(map);

  VLOG(1) << "BraveShieldsP3A: Domain FP counts: allow=" << fp_counts.allow
          << " standard=" << fp_counts.standard
          << " agg=" << fp_counts.aggressive;
  VLOG(1) << "BraveShieldsP3A: Domain Ad counts: allow=" << ads_counts.allow
          << " standard=" << ads_counts.standard
          << " agg=" << ads_counts.aggressive;

  UpdateDomainSettingCount(profile_prefs, true, ControlType::ALLOW,
                           fp_counts.allow);
  UpdateDomainSettingCount(profile_prefs, true, ControlType::DEFAULT,
                           fp_counts.standard);
  UpdateDomainSettingCount(profile_prefs, true, ControlType::BLOCK,
                           fp_counts.aggressive);

  UpdateDomainSettingCount(profile_prefs, false, ControlType::ALLOW,
                           ads_counts.allow);
  UpdateDomainSettingCount(profile_prefs, false, ControlType::BLOCK_THIRD_PARTY,
                           ads_counts.standard);
  UpdateDomainSettingCount(profile_prefs, false, ControlType::BLOCK,
                           ads_counts.aggressive);

  RecordShieldsDomainSettingCounts(profile_prefs, false, global_ads_setting);
  RecordShieldsDomainSettingCounts(profile_prefs, true, global_fp_setting);
  profile_prefs->SetBoolean(kFirstReportedPrefName, true);
}

void RegisterShieldsP3ALocalPrefs(PrefRegistrySimple* local_state) {
  local_state->RegisterIntegerPref(kUsagePrefName, -1);
}

void RegisterShieldsP3AProfilePrefs(PrefRegistrySimple* profile_state) {
  profile_state->RegisterBooleanPref(kFirstReportedPrefName, false);
  profile_state->RegisterIntegerPref(kAdsStrictCountPrefName, 0);
  profile_state->RegisterIntegerPref(kAdsStandardCountPrefName, 0);
  profile_state->RegisterIntegerPref(kAdsAllowCountPrefName, 0);
  profile_state->RegisterIntegerPref(kFPStrictCountPrefName, 0);
  profile_state->RegisterIntegerPref(kFPStandardCountPrefName, 0);
  profile_state->RegisterIntegerPref(kFPAllowCountPrefName, 0);
}

}  // namespace brave_shields
