/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/command_line_permission_rule.h"

#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/flags/flag_manager_util.h"

namespace ads {

namespace {

bool DoesRespectCap() {
  return !(IsProductionEnvironment() &&
           FlagManager::GetInstance()->DidOverrideFromCommandLine());
}

}  // namespace

bool CommandLinePermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Command-line arg is not supported";
    return false;
  }

  return true;
}

const std::string& CommandLinePermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
