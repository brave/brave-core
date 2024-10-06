/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/background_helper/background_helper_holder.h"

#include "base/no_destructor.h"
#include "brave/components/brave_ads/browser/application_state/background_helper.h"
#include "build/build_config.h"  // IWYU pragma: keep

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_ads/application_state/background_helper/background_helper_android.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_LINUX)
#include "brave/browser/brave_ads/application_state/background_helper/background_helper_linux.h"
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_MAC)
#include "brave/browser/brave_ads/application_state/background_helper/background_helper_mac.h"
#endif  // BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_ads/application_state/background_helper/background_helper_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace brave_ads {

BackgroundHelperHolder::BackgroundHelperHolder() {
#if BUILDFLAG(IS_ANDROID)
  background_helper_.reset(new BackgroundHelperAndroid());
#elif BUILDFLAG(IS_LINUX)
  background_helper_.reset(new BackgroundHelperLinux());
#elif BUILDFLAG(IS_MAC)
  background_helper_.reset(new BackgroundHelperMac());
#elif BUILDFLAG(IS_WIN)
  background_helper_.reset(new BackgroundHelperWin());
#else
  // Default background helper for unsupported platforms
  background_helper_.reset(new BackgroundHelper());
#endif  // BUILDFLAG(IS_ANDROID)
}

BackgroundHelperHolder::~BackgroundHelperHolder() = default;

// static
BackgroundHelperHolder* BackgroundHelperHolder::GetInstance() {
  static base::NoDestructor<BackgroundHelperHolder> instance;
  return instance.get();
}

BackgroundHelper* BackgroundHelperHolder::GetBackgroundHelper() {
  return background_helper_.get();
}

}  // namespace brave_ads
