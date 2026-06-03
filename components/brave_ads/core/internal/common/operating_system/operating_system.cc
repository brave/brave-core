/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"

#include "base/check_is_test.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_android.h"
#elif BUILDFLAG(IS_IOS)
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_ios.h"
#elif BUILDFLAG(IS_LINUX)
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_linux.h"
#elif BUILDFLAG(IS_MAC)
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_mac.h"
#elif BUILDFLAG(IS_WIN)
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_win.h"
#endif

namespace brave_ads {

namespace {

constexpr char kOperatingSystemName[] = "unknown";

const OperatingSystem* g_operating_system_for_testing = nullptr;

}  // namespace

OperatingSystem::OperatingSystem() = default;

OperatingSystem::~OperatingSystem() = default;

// static
void OperatingSystem::SetForTesting(  // IN-TEST
    const OperatingSystem* const operating_system) {
  CHECK_IS_TEST();

  g_operating_system_for_testing = operating_system;
}

std::string OperatingSystem::GetName() const {
  return kOperatingSystemName;
}

OperatingSystemType OperatingSystem::GetType() const {
  return OperatingSystemType::kUnknown;
}

// static
const OperatingSystem& OperatingSystem::GetInstance() {
  if (g_operating_system_for_testing) {
    CHECK_IS_TEST();

    return *g_operating_system_for_testing;
  }

#if BUILDFLAG(IS_ANDROID)
  static const base::NoDestructor<OperatingSystemAndroid> kOperatingSystem;
#elif BUILDFLAG(IS_IOS)
  static const base::NoDestructor<OperatingSystemIos> kOperatingSystem;
#elif BUILDFLAG(IS_LINUX)
  static const base::NoDestructor<OperatingSystemLinux> kOperatingSystem;
#elif BUILDFLAG(IS_MAC)
  static const base::NoDestructor<OperatingSystemMac> kOperatingSystem;
#elif BUILDFLAG(IS_WIN)
  static const base::NoDestructor<OperatingSystemWin> kOperatingSystem;
#else
  static const base::NoDestructor<OperatingSystem> kOperatingSystem;
#endif  // BUILDFLAG(IS_ANDROID)

  return *kOperatingSystem;
}

}  // namespace brave_ads
