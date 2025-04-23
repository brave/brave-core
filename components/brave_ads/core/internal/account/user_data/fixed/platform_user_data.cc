/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/platform_user_data.h"

#include <string>

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr char kPlatformKey[] = "platform";
}  // namespace

base::Value::Dict BuildPlatformUserData() {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  const std::string platform_name = PlatformHelper::GetInstance().GetName();
  if (platform_name.empty()) {
    // Invalid platform name.
    return {};
  }

  return base::Value::Dict().Set(kPlatformKey, platform_name);
}

}  // namespace brave_ads
