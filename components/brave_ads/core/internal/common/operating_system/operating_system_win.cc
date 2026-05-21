/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_win.h"

namespace brave_ads {

namespace {
constexpr char kOperatingSystemName[] = "windows";
}  // namespace

OperatingSystemWin::OperatingSystemWin() = default;

std::string OperatingSystemWin::GetName() const {
  return kOperatingSystemName;
}

OperatingSystemType OperatingSystemWin::GetType() const {
  return OperatingSystemType::kWindows;
}

}  // namespace brave_ads
