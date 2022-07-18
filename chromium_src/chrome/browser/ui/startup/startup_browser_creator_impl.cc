/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/buildflags/buildflags.h"
#include "chrome/browser/devtools/devtools_toggle_action.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/ui/browser_window.h"

#if BUILDFLAG(ENABLE_INSTANT_NEW_TAB)
#include "brave/browser/new_tab/brave_new_tab_service.h"
#include "brave/browser/new_tab/brave_new_tab_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_INSTANT_NEW_TAB)
void PreloadBraveNewTab(content::BrowserContext* browser_context) {
  auto* new_tab_cache_service =
      BraveNewTabServiceFactory::GetInstance()->GetServiceForContext(
          browser_context);
  if (!new_tab_cache_service)
    return;
  new_tab_cache_service->PreloadNewTab();
}

#define Show                              \
  Show();                                 \
  PreloadBraveNewTab(browser->profile()); \
  base::NullCallback
#endif
#define BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINEURLSANDLAUNCH \
  welcome_enabled = true;

#include "src/chrome/browser/ui/startup/startup_browser_creator_impl.cc"
#undef BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINEURLSANDLAUNCH
#if BUILDFLAG(ENABLE_INSTANT_NEW_TAB)
#undef Show
#endif
