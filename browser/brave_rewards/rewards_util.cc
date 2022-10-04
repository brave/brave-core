/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_util.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards {

bool IsSupportedForProfile(Profile* profile, IsSupportedOptions options) {
  DCHECK(profile);
  return brave::IsRegularProfile(profile) &&
         IsSupported(profile->GetPrefs(), options);
}

}  // namespace brave_rewards
