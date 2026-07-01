/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/safe_browsing/brave_generated_safe_browsing_pref.h"

#include "base/values.h"
#include "brave/components/safebrowsing/constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"

namespace safe_browsing {

BraveGeneratedSafeBrowsingPref::BraveGeneratedSafeBrowsingPref(Profile* profile)
    : GeneratedSafeBrowsingPref(profile), profile_(profile) {}

BraveGeneratedSafeBrowsingPref::~BraveGeneratedSafeBrowsingPref() = default;

extensions::settings_private::SetPrefResult
BraveGeneratedSafeBrowsingPref::SetPref(const base::Value* value) {
  // Intercept Limited before upstream rejects it as an out-of-range enum value.
  if (value->is_int() &&
      value->GetInt() == kBraveSafeBrowsingLimitedProtection) {
    const PrefService::Preference* enabled_pref =
        profile_->GetPrefs()->FindPreference(prefs::kSafeBrowsingEnabled);
    const PrefService::Preference* download_protection_pref =
        profile_->GetPrefs()->FindPreference(
            kBraveSafeBrowsingDownloadProtectionEnabled);
    if (!enabled_pref->IsUserModifiable() ||
        !download_protection_pref->IsUserModifiable()) {
      return extensions::settings_private::SetPrefResult::PREF_NOT_MODIFIABLE;
    }
    profile_->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, true);
    profile_->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnhanced, false);
    profile_->GetPrefs()->SetBoolean(
        kBraveSafeBrowsingDownloadProtectionEnabled, false);
    return extensions::settings_private::SetPrefResult::SUCCESS;
  }

  // Any other selection clears Limited by re-enabling download protection, but
  // only if upstream accepted it so a rejected write leaves the pref untouched.
  const extensions::settings_private::SetPrefResult result =
      GeneratedSafeBrowsingPref::SetPref(value);
  if (result == extensions::settings_private::SetPrefResult::SUCCESS) {
    profile_->GetPrefs()->SetBoolean(
        kBraveSafeBrowsingDownloadProtectionEnabled, true);
  }
  return result;
}

extensions::api::settings_private::PrefObject
BraveGeneratedSafeBrowsingPref::GetPrefObject() const {
  extensions::api::settings_private::PrefObject pref_object =
      GeneratedSafeBrowsingPref::GetPrefObject();
  // Report Limited when SB is Standard but download protection is off. Only
  // STANDARD matters: ENHANCED is unreachable in Brave, NO_SAFE_BROWSING makes
  // download protection moot. Safe to patch the value after the base call since
  // upstream's management-state pass never reads it.
  if (GetSafeBrowsingState(*profile_->GetPrefs()) ==
          SafeBrowsingState::STANDARD_PROTECTION &&
      !profile_->GetPrefs()->GetBoolean(
          kBraveSafeBrowsingDownloadProtectionEnabled)) {
    pref_object.value = base::Value(kBraveSafeBrowsingLimitedProtection);
  }
  return pref_object;
}

}  // namespace safe_browsing
