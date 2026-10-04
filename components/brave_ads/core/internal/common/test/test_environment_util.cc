/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/test_environment_util.h"

#include "base/check.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads::test {

namespace {

constexpr char kNightlyBuildChannelName[] = "nightly";
constexpr char kBetaBuildChannelName[] = "beta";
constexpr char kReleaseBuildChannelName[] = "release";

}  // namespace

void SetUpDeviceId() {
  CHECK(GlobalState::HasInstance());

  GlobalState::GetInstance()->SysInfo().device_id = kDeviceId;
}

void SetUpBuildChannel(BuildChannelType type) {
  CHECK(GlobalState::HasInstance());

  auto& build_channel = GlobalState::GetInstance()->BuildChannel();
  switch (type) {
    case BuildChannelType::kNightly: {
      build_channel.is_release = false;
      build_channel.name = kNightlyBuildChannelName;
      return;
    }

    case BuildChannelType::kBeta: {
      build_channel.is_release = false;
      build_channel.name = kBetaBuildChannelName;
      return;
    }

    case BuildChannelType::kRelease: {
      build_channel.is_release = true;
      build_channel.name = kReleaseBuildChannelName;
      return;
    }
  }

  NOTREACHED() << "Unexpected value for BuildChannelType: "
               << std::to_underlying(type);
}

void SetUpAllowJavaScript(bool allow_javascript) {
  GlobalState::GetInstance()->ContentSettings().allow_javascript =
      allow_javascript;
}

}  // namespace brave_ads::test
