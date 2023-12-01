/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_info.h"

namespace brave_ads {

void InitializeCommandLineSwitches();
void ShutdownCommandLineSwitches();

std::optional<bool>& DidAppendCommandLineSwitches();
void AppendCommandLineSwitches(
    const CommandLineSwitchList& command_line_switches);

std::string SanitizeCommandLineSwitch(
    const CommandLineSwitchInfo& command_line_switch);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_UTIL_H_
