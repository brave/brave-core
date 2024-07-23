/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/command_line_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_flag_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_flag_util.h"

namespace brave_ads {

bool HasCommandLinePermission() {
  if (!IsProductionEnvironment()) {
    // Always respect cap for staging environment.
    return true;
  }

  if (!DidOverrideCommandLine()) {
    return true;
  }

  BLOG(2, "Command-line arg is not supported");
  return false;
}

}  // namespace brave_ads
