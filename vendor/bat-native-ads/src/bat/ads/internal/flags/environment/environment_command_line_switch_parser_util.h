/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_ENVIRONMENT_ENVIRONMENT_COMMAND_LINE_SWITCH_PARSER_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_ENVIRONMENT_ENVIRONMENT_COMMAND_LINE_SWITCH_PARSER_UTIL_H_

#include "absl/types/optional.h"
#include "bat/ads/internal/flags/environment/environment_types.h"

namespace ads {

absl::optional<EnvironmentType> ParseEnvironmentCommandLineSwitch();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_ENVIRONMENT_ENVIRONMENT_COMMAND_LINE_SWITCH_PARSER_UTIL_H_
