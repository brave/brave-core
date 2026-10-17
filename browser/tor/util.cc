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
  // Intentionally not `IncognitoModePrefs::GetAvailability()`: it permanently
  // caches the profile's `IsolatedModeSettingsService`, and this runs at
  // startup before enterprise policy has settled. Isolated Mode only moves
  // availability towards kEnabled, which is therefore ignored here.
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
