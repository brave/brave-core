/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/util.h"

#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace tor {

bool IsIncognitoDisabledOrForced(content::BrowserContext* context) {
  const auto availability =
      IncognitoModePrefs::GetAvailability(user_prefs::UserPrefs::Get(context));
  return availability == policy::IncognitoModeAvailability::kDisabled ||
         availability == policy::IncognitoModeAvailability::kForced;
}

}  // namespace tor
