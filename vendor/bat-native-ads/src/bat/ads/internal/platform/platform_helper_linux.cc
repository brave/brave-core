/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/platform/platform_helper_linux.h"

#include "base/memory/singleton.h"

namespace ads {

PlatformHelperLinux::PlatformHelperLinux() = default;

PlatformHelperLinux::~PlatformHelperLinux() = default;

bool PlatformHelperLinux::IsMobile() const {
  return false;
}

std::string PlatformHelperLinux::GetPlatformName() const {
  return "linux";
}

PlatformType PlatformHelperLinux::GetPlatform() const {
  return PlatformType::kLinux;
}

PlatformHelperLinux* PlatformHelperLinux::GetInstanceImpl() {
  return base::Singleton<PlatformHelperLinux>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperLinux::GetInstanceImpl();
}

}  // namespace ads
