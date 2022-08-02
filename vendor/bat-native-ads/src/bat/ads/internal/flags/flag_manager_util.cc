/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/flag_manager_util.h"

#include "bat/ads/internal/flags/flag_manager.h"

namespace ads {

bool ShouldDebug() {
  return FlagManager::GetInstance()->ShouldDebug();
}

void SetShouldDebugForTesting(const bool should_debug) {
  FlagManager::GetInstance()->SetShouldDebugForTesting(should_debug);
}

bool DidOverrideFromCommandLine() {
  return FlagManager::GetInstance()->DidOverrideFromCommandLine();
}

void SetDidOverrideFromCommandLineForTesting(
    const bool did_override_from_command_line) {
  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(
      did_override_from_command_line);
}

EnvironmentType GetEnvironmentType() {
  return FlagManager::GetInstance()->GetEnvironmentType();
}

bool IsProductionEnvironment() {
  return FlagManager::GetInstance()->GetEnvironmentType() ==
         EnvironmentType::kProduction;
}

void SetEnvironmentTypeForTesting(const EnvironmentType environment_type) {
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(environment_type);
}

}  // namespace ads
