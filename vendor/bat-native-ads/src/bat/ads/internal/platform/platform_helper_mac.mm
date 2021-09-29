/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/platform/platform_helper_mac.h"

#include "base/memory/singleton.h"

namespace ads {

PlatformHelperMac::PlatformHelperMac() = default;

PlatformHelperMac::~PlatformHelperMac() = default;

bool PlatformHelperMac::IsMobile() const {
  return false;
}

std::string PlatformHelperMac::GetPlatformName() const {
  return "macos";
}

PlatformType PlatformHelperMac::GetPlatform() const {
  return PlatformType::kMacOS;
}

PlatformHelperMac* PlatformHelperMac::GetInstanceImpl() {
  return base::Singleton<PlatformHelperMac>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperMac::GetInstanceImpl();
}

}  // namespace ads
