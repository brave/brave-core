/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/platform_helper_win.h"

namespace confirmations {

PlatformHelperWin::PlatformHelperWin() = default;

PlatformHelperWin::~PlatformHelperWin() = default;

std::string PlatformHelperWin::GetPlatformName() const {
  return "windows";
}

PlatformHelperWin* PlatformHelperWin::GetInstanceImpl() {
  return base::Singleton<PlatformHelperWin>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperWin::GetInstanceImpl();
}

}  // namespace confirmations
