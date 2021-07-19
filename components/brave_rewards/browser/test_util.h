/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_UTIL_H_

#include <memory>

#include "base/files/file_path.h"

class KeyedService;
class Profile;

namespace brave_rewards {

std::unique_ptr<Profile> CreateBraveRewardsProfile(
    const base::FilePath& path,
    const int country_id = 'U' << 8 | 'S');

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_UTIL_H_
