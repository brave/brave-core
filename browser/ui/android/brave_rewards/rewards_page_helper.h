// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_ANDROID_BRAVE_REWARDS_REWARDS_PAGE_HELPER_H_
#define BRAVE_BROWSER_UI_ANDROID_BRAVE_REWARDS_REWARDS_PAGE_HELPER_H_

#include <string>

#include "brave/components/brave_rewards/core/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_REWARDS));

namespace brave_rewards {

// Opens any other URL
void OpenURLForRewardsPage(const std::string& url);

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_ANDROID_BRAVE_REWARDS_REWARDS_PAGE_HELPER_H_
