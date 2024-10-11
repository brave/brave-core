/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

#include "base/check_is_test.h"
#include "build/build_config.h"
#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_android.h"
#elif BUILDFLAG(IS_IOS)
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_ios.h"
#elif BUILDFLAG(IS_LINUX)
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_linux.h"
#elif BUILDFLAG(IS_MAC)
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_mac.h"
#elif BUILDFLAG(IS_WIN)
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_win.h"
#endif

namespace brave_ads {

namespace {

constexpr char kPlatformName[] = "unknown";

const PlatformHelper* g_platform_helper_for_testing = nullptr;

}  // namespace

PlatformHelper::PlatformHelper() = default;

PlatformHelper::~PlatformHelper() = default;

// static
void PlatformHelper::SetForTesting(
    const PlatformHelper* const platform_helper) {
  CHECK_IS_TEST();

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
const PlatformHelper& PlatformHelper::GetInstance() {
  if (g_platform_helper_for_testing) {
    CHECK_IS_TEST();

    return *g_platform_helper_for_testing;
  }

  return GetInstanceImpl();
}

// static
const PlatformHelper& PlatformHelper::GetInstanceImpl() {
#if BUILDFLAG(IS_ANDROID)
  static const base::NoDestructor<PlatformHelperAndroid> kPlatformHelper;
#elif BUILDFLAG(IS_IOS)
  static const base::NoDestructor<PlatformHelperIos> kPlatformHelper;
#elif BUILDFLAG(IS_LINUX)
  static const base::NoDestructor<PlatformHelperLinux> kPlatformHelper;
#elif BUILDFLAG(IS_MAC)
  static const base::NoDestructor<PlatformHelperMac> kPlatformHelper;
#elif BUILDFLAG(IS_WIN)
  static const base::NoDestructor<PlatformHelperWin> kPlatformHelper;
#else
  // Default platform helper for unsupported platforms
  static const base::NoDestructor<PlatformHelper> kPlatformHelper;
#endif  // BUILDFLAG(IS_ANDROID)

  return *kPlatformHelper;
}

}  // namespace brave_ads
