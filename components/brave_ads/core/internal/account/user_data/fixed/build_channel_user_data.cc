/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/build_channel_user_data.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr char kBuildChannelKey[] = "buildChannel";
}  // namespace

base::Value::Dict BuildBuildChannelUserData() {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  base::Value::Dict user_data;

  const auto& build_channel = GlobalState::GetInstance()->BuildChannel();
  CHECK(!build_channel.name.empty());

  user_data.Set(kBuildChannelKey, build_channel.name);

  return user_data;
}

}  // namespace brave_ads
