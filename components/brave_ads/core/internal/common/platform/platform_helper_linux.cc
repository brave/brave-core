/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_linux.h"

namespace brave_ads {

namespace {
constexpr char kPlatformName[] = "linux";
}  // namespace

PlatformHelperLinux::PlatformHelperLinux() = default;

bool PlatformHelperLinux::IsMobile() const {
  return false;
}

std::string PlatformHelperLinux::GetName() const {
  return kPlatformName;
}

PlatformType PlatformHelperLinux::GetType() const {
  return PlatformType::kLinux;
}

}  // namespace brave_ads
