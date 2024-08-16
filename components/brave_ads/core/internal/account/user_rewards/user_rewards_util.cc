/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

bool ShouldMigrateVerifiedRewardsUser() {
  return UserHasJoinedBraveRewards() &&
         GetProfileBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser);
}

void UpdateIssuers(const IssuersInfo& issuers) {
  if (!HasIssuersChanged(issuers)) {
    return BLOG(1, "Issuers already up to date");
  }

  BLOG(1, "Updated issuers");
  SetIssuers(issuers);
}

}  // namespace brave_ads
