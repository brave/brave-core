/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/platform/platform_helper_win.h"

#include "base/memory/singleton.h"

namespace ads {

PlatformHelperWin::PlatformHelperWin() = default;

PlatformHelperWin::~PlatformHelperWin() = default;

bool PlatformHelperWin::IsMobile() const {
  return false;
}

std::string PlatformHelperWin::GetPlatformName() const {
  return "windows";
}

PlatformType PlatformHelperWin::GetPlatform() const {
  return PlatformType::kWindows;
}

PlatformHelperWin* PlatformHelperWin::GetInstanceImpl() {
  return base::Singleton<PlatformHelperWin>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperWin::GetInstanceImpl();
}

}  // namespace ads
