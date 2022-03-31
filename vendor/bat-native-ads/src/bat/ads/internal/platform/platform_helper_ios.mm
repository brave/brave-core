/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/platform/platform_helper_ios.h"

#include "base/memory/singleton.h"

namespace ads {

PlatformHelperIos::PlatformHelperIos() = default;

PlatformHelperIos::~PlatformHelperIos() = default;

bool PlatformHelperIos::IsMobile() const {
  return true;
}

std::string PlatformHelperIos::GetPlatformName() const {
  return "ios";
}

PlatformType PlatformHelperIos::GetPlatform() const {
  return PlatformType::kIOS;
}

PlatformHelperIos* PlatformHelperIos::GetInstanceImpl() {
  return base::Singleton<PlatformHelperIos>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperIos::GetInstanceImpl();
}

}  // namespace ads
