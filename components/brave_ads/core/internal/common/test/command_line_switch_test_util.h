/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_COMMAND_LINE_SWITCH_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_COMMAND_LINE_SWITCH_TEST_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_info.h"

namespace brave_ads::test {

void SimulateCommandLineSwitches();
void ResetCommandLineSwitches();

std::optional<bool>& DidAppendCommandLineSwitches();

// Should only be called from `test::TestBase::SetUpMocks`.
void AppendCommandLineSwitches(
    const CommandLineSwitchList& command_line_switches);

std::string ToString(const CommandLineSwitchInfo& command_line_switch);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_COMMAND_LINE_SWITCH_TEST_UTIL_H_
