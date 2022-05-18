/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/platform_helper.h"

#include "build/build_config.h"
#if BUILDFLAG(IS_ANDROID)
#include "bat/ads/internal/base/platform_helper_android.h"
#elif BUILDFLAG(IS_IOS)
#include "bat/ads/internal/base/platform_helper_ios.h"
#elif BUILDFLAG(IS_LINUX)
#include "bat/ads/internal/base/platform_helper_linux.h"
#elif BUILDFLAG(IS_MAC)
#include "bat/ads/internal/base/platform_helper_mac.h"
#elif BUILDFLAG(IS_WIN)
#include "bat/ads/internal/base/platform_helper_win.h"
#endif

namespace ads {

namespace {
constexpr char kPlatformName[] = "unknown";
}  // namespace

PlatformHelper* g_platform_helper_for_testing = nullptr;

PlatformHelper::PlatformHelper() = default;

PlatformHelper::~PlatformHelper() = default;

void PlatformHelper::SetForTesting(PlatformHelper* platform_helper) {
  g_platform_helper_for_testing = platform_helper;
}

bool PlatformHelper::IsMobile() const {
  return false;
}

std::string PlatformHelper::GetName() const {
  return kPlatformName;
}

PlatformType PlatformHelper::GetType() const {
  return PlatformType::kUnknown;
}

// static
PlatformHelper* PlatformHelper::GetInstance() {
  if (g_platform_helper_for_testing) {
    return g_platform_helper_for_testing;
  }

  return GetInstanceImpl();
}

// static
PlatformHelper* PlatformHelper::GetInstanceImpl() {
#if BUILDFLAG(IS_ANDROID)
  static base::NoDestructor<PlatformHelperAndroid> platform_helper;
#elif BUILDFLAG(IS_IOS)
  static base::NoDestructor<PlatformHelperIos> platform_helper;
#elif BUILDFLAG(IS_LINUX)
  static base::NoDestructor<PlatformHelperLinux> platform_helper;
#elif BUILDFLAG(IS_MAC)
  static base::NoDestructor<PlatformHelperMac> platform_helper;
#elif BUILDFLAG(IS_WIN)
  static base::NoDestructor<PlatformHelperWin> platform_helper;
#else
  // Default platform helper for unsupported platforms
  static base::NoDestructor<PlatformHelper> platform_helper;
#endif  // BUILDFLAG(IS_ANDROID)

  return platform_helper.get();
}

}  // namespace ads
