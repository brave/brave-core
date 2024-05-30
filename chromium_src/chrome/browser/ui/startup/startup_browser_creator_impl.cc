/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"
#include "build/build_config.h"
#include "chrome/browser/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_tab_provider.h"
#include "components/signin/public/base/signin_buildflags.h"

#if BUILDFLAG(ENABLE_UPDATER)
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/cocoa/keystone_infobar_delegate.h"
#include "chrome/browser/ui/startup/startup_types.h"
#endif

#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_UPDATER)
namespace {
void MaybeShowPromotionInfoBar(
    chrome::startup::IsProcessStartup process_startup) {
  if (process_startup == chrome::startup::IsProcessStartup::kYes) {
    // Check whether the auto-update system needs to be promoted from user
    // to system.
    ShowUpdaterPromotionInfoBar();
  }
}
}  // namespace

#define GetLastActive                         \
  GetLastActive();                            \
  MaybeShowPromotionInfoBar(process_startup); \
  BrowserList::GetInstance()->GetLastActive
#endif

#define StartupTabProviderImpl BraveStartupTabProviderImpl

#include "src/chrome/browser/ui/startup/startup_browser_creator_impl.cc"

#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_UPDATER)
#undef GetLastActive
#endif

#undef StartupTabProviderImpl
