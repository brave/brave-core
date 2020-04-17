/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/platform_helper.h"

namespace confirmations {

PlatformHelper* g_platform_helper_for_testing = nullptr;

PlatformHelper::PlatformHelper() = default;

PlatformHelper::~PlatformHelper() = default;

void PlatformHelper::set_for_testing(
    PlatformHelper* platform_helper) {
  g_platform_helper_for_testing = platform_helper;
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

#if !defined(OS_MACOSX) && \
    !defined(OS_WIN) && \
    !defined(OS_LINUX) && \
    !defined(OS_ANDROID) && \
    !defined(OS_IOS)
PlatformHelper* PlatformHelper::GetInstanceImpl() {
  // Return a default platform helper for unsupported platforms
  return base::Singleton<PlatformHelper>::get();
}
#endif

}  // namespace confirmations
