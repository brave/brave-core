/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_UNITTEST_COMMAND_LINE_SWITCH_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_UNITTEST_COMMAND_LINE_SWITCH_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct CommandLineSwitchInfo final {
  CommandLineSwitchInfo();
  CommandLineSwitchInfo(const std::string& key, const std::string& value);
  CommandLineSwitchInfo(const CommandLineSwitchInfo& info);
  CommandLineSwitchInfo& operator=(const CommandLineSwitchInfo& info);
  ~CommandLineSwitchInfo();

  std::string key;
  std::string value;
};

using CommandLineSwitchList = std::vector<CommandLineSwitchInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_UNITTEST_COMMAND_LINE_SWITCH_INFO_H_
