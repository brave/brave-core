/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/util.h"

#include "base/check.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

namespace tor {

bool IsIncognitoDisabledOrForced(content::BrowserContext* context) {
  // Deliberately avoids `IncognitoModePrefs::GetAvailability()`: since Chromium
  // 156 it also evaluates and permanently caches enterprise_isolated_mode's
  // `IsolatedModeSettingsService` for the profile (see
  // isolated_mode_settings_service.cc). This is called from
  // `BraveExtensionManagement`'s startup Tor cleanup, which can run before
  // enterprise policy (including cloud-delivered Isolated Mode policy) has
  // settled, so triggering that cache here would lock in a stale value for the
  // rest of the profile's lifetime. Isolated Mode can only push the effective
  // availability towards kEnabled, never towards kDisabled/kForced, so it's
  // irrelevant to the value computed here.
  const PrefService* prefs = Profile::FromBrowserContext(context)->GetPrefs();
  policy::IncognitoModeAvailability availability;
  bool valid = IncognitoModePrefs::IntToAvailability(
      prefs->GetInteger(policy::policy_prefs::kIncognitoModeAvailability),
      &availability);
  DCHECK(valid);
  if (availability != policy::IncognitoModeAvailability::kDisabled &&
      IncognitoModePrefs::ArePlatformParentalControlsEnabled()) {
    availability = policy::IncognitoModeAvailability::kDisabled;
  }
  return availability == policy::IncognitoModeAvailability::kDisabled ||
         availability == policy::IncognitoModeAvailability::kForced;
}

}  // namespace tor
