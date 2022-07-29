/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_FLAG_MANAGER_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_FLAG_MANAGER_UTIL_H_

#include "bat/ads/internal/flags/environment/environment_types.h"

namespace ads {

bool ShouldDebug();
void SetShouldDebugForTesting(const bool should_debug);

bool DidOverrideVariationsCommandLineSwitches();
void SetDidOverrideVariationsCommandLineSwitchesForTesting(
    const bool did_override_variations_command_line_switches);

EnvironmentType GetEnvironmentType();
bool IsProductionEnvironment();
void SetEnvironmentTypeForTesting(const EnvironmentType environment_type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_FLAG_MANAGER_UTIL_H_
