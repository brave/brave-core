/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/platform_helper_mac.h"

namespace confirmations {

PlatformHelperMac::PlatformHelperMac() = default;

PlatformHelperMac::~PlatformHelperMac() = default;

std::string PlatformHelperMac::GetPlatformName() const {
  return "macos";
}

PlatformHelperMac* PlatformHelperMac::GetInstanceImpl() {
  return base::Singleton<PlatformHelperMac>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperMac::GetInstanceImpl();
}

}  // namespace confirmations
