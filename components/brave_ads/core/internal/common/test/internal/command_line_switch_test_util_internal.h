/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_COMMAND_LINE_SWITCH_TEST_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_COMMAND_LINE_SWITCH_TEST_UTIL_INTERNAL_H_

#include <optional>

namespace brave_ads::test {

void SimulateCommandLineSwitches();
void ResetCommandLineSwitches();

std::optional<bool>& DidAppendCommandLineSwitches();

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_COMMAND_LINE_SWITCH_TEST_UTIL_INTERNAL_H_
