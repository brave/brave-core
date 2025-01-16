/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_TEST_UTIL_H_

#include <memory>

#include "base/files/file_path.h"

class Profile;

namespace brave_rewards {

std::unique_ptr<Profile> CreateBraveRewardsProfile(const base::FilePath& path);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_TEST_UTIL_H_
