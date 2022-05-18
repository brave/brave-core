/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/platform_helper_linux.h"

namespace ads {

namespace {
constexpr char kPlatformName[] = "linux";
}  // namespace

PlatformHelperLinux::PlatformHelperLinux() = default;

PlatformHelperLinux::~PlatformHelperLinux() = default;

bool PlatformHelperLinux::IsMobile() const {
  return false;
}

std::string PlatformHelperLinux::GetName() const {
  return kPlatformName;
}

PlatformType PlatformHelperLinux::GetType() const {
  return PlatformType::kLinux;
}

}  // namespace ads
