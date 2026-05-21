/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/test_environment_util_internal.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/command_line_switch_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/command_line_switches/command_line_switches_util.h"

namespace brave_ads::test {

void SetUpCommandLineSwitches() {
  CHECK(GlobalState::HasInstance());

  GlobalState::GetInstance()->CommandLineSwitches() =
      *BuildCommandLineSwitches();

  if (!DidAppendCommandLineSwitches()) {
    // Force the test environment to staging if we did not append command-line
    // switches in `SetUpMocks`, or if the test environment does not support
    // passing command-line switches.
    GlobalState::GetInstance()->CommandLineSwitches().environment_type =
        mojom::EnvironmentType::kStaging;
  }
}

void SetUpContentSettings() {
  GlobalState::GetInstance()->ContentSettings().allow_javascript = true;
}

}  // namespace brave_ads::test
