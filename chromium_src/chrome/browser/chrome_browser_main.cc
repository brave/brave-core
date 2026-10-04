/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_main.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/updater/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/metrics/chrome_browser_main_extra_parts_metrics.h"
#include "chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.h"

#if BUILDFLAG(ENABLE_OMAHA4)
#include "brave/browser/updater/features.h"
#endif  // BUILDFLAG(ENABLE_OMAHA4)

#if BUILDFLAG(IS_MAC)
#include "brave/browser/brave_browser_main_parts_mac.h"
#undef ChromeBrowserMainPartsMac
#define ChromeBrowserMainPartsMac BraveBrowserMainPartsMac
#endif  // BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_UPDATER)
namespace {

// Most macOS users are still updated by Sparkle, which manages promotion
// itself, so the Keystone promotion infobar must not reach them.
bool ShouldPromptUpdaterPromotion() {
#if BUILDFLAG(ENABLE_OMAHA4)
  return brave_updater::ShouldUseOmaha4();
#else
  return true;
#endif
}

}  // namespace
#endif  // BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_UPDATER)

#define BrowserProcessImpl BraveBrowserProcessImpl
#define ChromeBrowserMainParts ChromeBrowserMainParts_ChromiumImpl
#include <chrome/browser/chrome_browser_main.cc>
#undef ChromeBrowserMainParts
#undef BrowserProcessImpl

#if BUILDFLAG(IS_MAC)
#undef ChromeBrowserMainPartsMac
#endif  // BUILDFLAG(IS_MAC)
