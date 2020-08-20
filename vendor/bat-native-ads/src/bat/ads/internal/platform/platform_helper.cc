/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/platform/platform_helper.h"

namespace ads {

PlatformHelper* g_platform_helper_for_testing = nullptr;

PlatformHelper::PlatformHelper() = default;

PlatformHelper::~PlatformHelper() = default;

void PlatformHelper::set_for_testing(
    PlatformHelper* platform_helper) {
  g_platform_helper_for_testing = platform_helper;
}

bool PlatformHelper::IsMobile() const {
  return false;
}

PlatformType PlatformHelper::GetPlatform() const {
  return PlatformType::kUnknown;
}

std::string PlatformHelper::GetPlatformName() const {
  return "unknown";
}

PlatformHelper* PlatformHelper::GetInstance() {
  if (g_platform_helper_for_testing) {
    return g_platform_helper_for_testing;
  }

  return GetInstanceImpl();
}

#if !defined(OS_ANDROID) && \
    !defined(OS_APPLE) && \
    !defined(OS_LINUX) && \
    !defined(OS_WIN)
PlatformHelper* PlatformHelper::GetInstanceImpl() {
  // Return a default platform helper for unsupported platforms
  return base::Singleton<PlatformHelper>::get();
}
#endif

}  // namespace ads
