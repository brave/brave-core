/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/command_line_permission_rule.h"

#include "bat/ads/internal/flags/flag_manager_util.h"

namespace ads {

CommandLinePermissionRule::CommandLinePermissionRule() = default;

CommandLinePermissionRule::~CommandLinePermissionRule() = default;

bool CommandLinePermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "--enable-features command-line arg is not supported";
    return false;
  }

  return true;
}

std::string CommandLinePermissionRule::GetLastMessage() const {
  return last_message_;
}

bool CommandLinePermissionRule::DoesRespectCap() {
  if (IsProductionEnvironment() && DidOverrideFromCommandLine()) {
    return false;
  }

  return true;
}

}  // namespace ads
