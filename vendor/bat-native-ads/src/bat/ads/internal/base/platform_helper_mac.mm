/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/platform_helper_mac.h"

namespace ads {

namespace {
constexpr char kPlatformName[] = "macos";
}  // namespace

PlatformHelperMac::PlatformHelperMac() = default;

PlatformHelperMac::~PlatformHelperMac() = default;

bool PlatformHelperMac::IsMobile() const {
  return false;
}

std::string PlatformHelperMac::GetName() const {
  return kPlatformName;
}

PlatformType PlatformHelperMac::GetType() const {
  return PlatformType::kMacOS;
}

}  // namespace ads
