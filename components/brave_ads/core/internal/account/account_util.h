/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_UTIL_H_

#include "base/functional/callback_forward.h"

namespace brave_ads {

using ResetRewardsCallback = base::OnceCallback<void(bool success)>;

bool UserHasOptedInToBravePrivateAds();
bool UserHasOptedInToBraveNews();
bool UserHasOptedInToNewTabPageAds();

bool ShouldRewardUser();

void ResetRewards(ResetRewardsCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_UTIL_H_
