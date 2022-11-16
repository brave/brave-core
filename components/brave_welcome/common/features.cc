/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_welcome/common/features.h"

#include "base/feature_list.h"

namespace brave_welcome {
namespace features {

BASE_FEATURE(kShowRewardsCard,
             "ShowRewardsCard",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
}  // namespace brave_welcome
