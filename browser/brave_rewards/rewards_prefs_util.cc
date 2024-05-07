/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_prefs_util.h"

#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards {

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  if (auto* old_show_button_pref = prefs->FindPreference(prefs::kShowButton);
      !old_show_button_pref->IsDefaultValue()) {
    // This preference was overloaded for showing the location bar button and
    // tipping buttons
    const bool value = prefs->GetBoolean(prefs::kShowButton);
    prefs->SetBoolean(prefs::kShowLocationBarButton, value);
  }

  prefs->ClearPref(prefs::kShowButton);

  // Added 05/2023
  prefs->ClearPref(prefs::kAdsWereDisabled);
  prefs->ClearPref(prefs::kHasAdsP3AState);

  // Added 07/2023
  prefs->ClearPref(prefs::kAdsEnabledTimestamp);
  prefs->ClearPref(prefs::kAdsEnabledTimeDelta);

  // Added 03/2023
  prefs->ClearPref(prefs::kFetchOldBalance);
  prefs->ClearPref(prefs::kEmptyBalanceChecked);
  prefs->ClearPref(prefs::kPromotionCorruptedMigrated);
  prefs->ClearPref(prefs::kPromotionLastFetchStamp);
}

}  // namespace brave_rewards
