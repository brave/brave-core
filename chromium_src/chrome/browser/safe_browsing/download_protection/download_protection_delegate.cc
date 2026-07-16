/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/safe_browsing/download_protection/download_protection_delegate.h"

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/safe_browsing/brave_download_protection_delegate_desktop.h"

#define DownloadProtectionDelegateDesktop BraveDownloadProtectionDelegateDesktop
#endif  // !BUILDFLAG(IS_ANDROID)

#include <chrome/browser/safe_browsing/download_protection/download_protection_delegate.cc>

#if !BUILDFLAG(IS_ANDROID)
#undef DownloadProtectionDelegateDesktop
#endif  // !BUILDFLAG(IS_ANDROID)
