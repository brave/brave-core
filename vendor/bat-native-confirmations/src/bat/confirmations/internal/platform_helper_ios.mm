/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/platform_helper_ios.h"

namespace confirmations {

PlatformHelperIos::PlatformHelperIos() = default;

PlatformHelperIos::~PlatformHelperIos() = default;

std::string PlatformHelperIos::GetPlatformName() const {
  return "ios";
}

PlatformHelperIos* PlatformHelperIos::GetInstanceImpl() {
  return base::Singleton<PlatformHelperIos>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperIos::GetInstanceImpl();
}

}  // namespace confirmations
