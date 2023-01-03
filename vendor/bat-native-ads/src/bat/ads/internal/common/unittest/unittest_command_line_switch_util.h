/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_UTIL_H_

#include <string>

#include "bat/ads/internal/common/unittest/command_line_switch_info.h"

namespace ads {

void InitializeCommandLineSwitches();
void CleanupCommandLineSwitches();

void AppendCommandLineSwitches(
    const CommandLineSwitchList& command_line_switches);

std::string SanitizeCommandLineSwitch(
    const CommandLineSwitchInfo& command_line_switch);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_UTIL_H_
