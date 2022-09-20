/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/unittest/command_line_switch_info.h"

namespace ads {

CommandLineSwitchInfo::CommandLineSwitchInfo() = default;

CommandLineSwitchInfo::CommandLineSwitchInfo(const std::string& key,
                                             const std::string& value)
    : key(key), value(value) {}

}  // namespace ads
