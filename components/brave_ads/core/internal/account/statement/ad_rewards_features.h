/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_AD_REWARDS_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_AD_REWARDS_FEATURES_H_

#include "base/feature_list.h"

namespace brave_ads::features {

BASE_DECLARE_FEATURE(kAdRewards);

bool IsAdRewardsEnabled();

int GetAdRewardsNextPaymentDay();

}  // namespace brave_ads::features

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_AD_REWARDS_FEATURES_H_
