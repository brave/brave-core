/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_INFO_H_

#include <string>
#include <vector>

namespace brave_ads {

struct CommandLineSwitchInfo final {
  CommandLineSwitchInfo();
  CommandLineSwitchInfo(std::string key, std::string value);

  std::string key;
  std::string value;
};

using CommandLineSwitchList = std::vector<CommandLineSwitchInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_COMMAND_LINE_SWITCH_INFO_H_
