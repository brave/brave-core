/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_prefs_util.h"

#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards {

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  const PrefService::Preference* deprecated_hide_button_pref =
      prefs->FindPreference(prefs::kHideButton);
  if (!deprecated_hide_button_pref) {
    return;
  }

  if (!deprecated_hide_button_pref->IsDefaultValue()) {
    prefs->SetBoolean(prefs::kShowButton,
                      !prefs->GetBoolean(prefs::kHideButton));
  }

  prefs->ClearPref(prefs::kHideButton);
}

}  // namespace brave_rewards
