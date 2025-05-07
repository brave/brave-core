/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/new_tab_takeover_infobar_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace ntp_background_images {

bool ShouldShowNewTabTakeoverInfobar(const PrefService* prefs) {
  CHECK(prefs);

  if (!brave_ads::kShouldSupportNewTabPageAdConfirmationsForNonRewards.Get()) {
    return false;
  }

  if (prefs->GetBoolean(brave_rewards::prefs::kEnabled)) {
    return false;
  }

  return prefs->GetInteger(prefs::kNewTabTakeoverInfobarShowCount) > 0;
}

void RecordNewTabTakeoverInfobarWasShown(PrefService* prefs) {
  CHECK(prefs);

  const int count = prefs->GetInteger(prefs::kNewTabTakeoverInfobarShowCount);
  prefs->SetInteger(prefs::kNewTabTakeoverInfobarShowCount, count - 1);
}

void SuppressNewTabTakeoverInfobar(PrefService* prefs) {
  CHECK(prefs);

  prefs->SetInteger(prefs::kNewTabTakeoverInfobarShowCount, 0);
}

}  // namespace ntp_background_images
