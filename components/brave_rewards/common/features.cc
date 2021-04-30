/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/features.h"

#include "base/feature_list.h"

namespace brave_rewards {
namespace features {

const base::Feature kBitflyerFeature{"BraveRewardsBitflyer",
                                     base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kVerboseLoggingFeature{"BraveRewardsVerboseLogging",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace brave_rewards
