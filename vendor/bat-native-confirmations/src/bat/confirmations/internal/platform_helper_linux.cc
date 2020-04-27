/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/platform_helper_linux.h"

namespace confirmations {

PlatformHelperLinux::PlatformHelperLinux() = default;

PlatformHelperLinux::~PlatformHelperLinux() = default;

std::string PlatformHelperLinux::GetPlatformName() const {
  return "linux";
}

PlatformHelperLinux* PlatformHelperLinux::GetInstanceImpl() {
  return base::Singleton<PlatformHelperLinux>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperLinux::GetInstanceImpl();
}

}  // namespace confirmations
