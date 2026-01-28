/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_tab_provider.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/ui/browser_list.h"
#endif

#define StartupTabProviderImpl BraveStartupTabProviderImpl

// On Windows, we need to override GetExistingBrowserForOpenBehavior to be
// workspace-aware. The upstream implementation uses FindLastActiveWithProfile
// which doesn't filter by the current virtual desktop. This causes external
// URLs to switch the user to a different virtual desktop instead of opening
// on the current one.
// See: https://github.com/brave/brave-browser/issues/52077
#if BUILDFLAG(IS_WIN)
#define GetExistingBrowserForOpenBehavior \
  GetExistingBrowserForOpenBehavior_ChromiumImpl
#endif

#include <chrome/browser/ui/startup/startup_browser_creator_impl.cc>

#if BUILDFLAG(IS_WIN)
#undef GetExistingBrowserForOpenBehavior

namespace {

// Windows-specific implementation that respects virtual desktop boundaries.
// This ensures external URLs open on the current virtual desktop instead of
// switching the user to a different desktop where an existing browser exists.
Browser* GetExistingBrowserForOpenBehavior(
    Profile* profile,
    chrome::startup::IsProcessStartup process_startup) {
  // First try to find a browser on the current workspace
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (browser->profile() == profile &&
        browser->type() == Browser::TYPE_NORMAL && browser->window() &&
        browser->window()->IsOnCurrentWorkspace()) {
      return browser;
    }
  }

  // If no browser on current workspace, fall back to chromium's behavior
  // but only for process startup (not for external URL handling)
  if (process_startup == chrome::startup::IsProcessStartup::kYes) {
    return GetExistingBrowserForOpenBehavior_ChromiumImpl(profile,
                                                          process_startup);
  }

  // For external URLs (non-process-startup), return nullptr to force
  // creation of a new window on the current virtual desktop
  return nullptr;
}

}  // namespace
#endif  // BUILDFLAG(IS_WIN)

#undef StartupTabProviderImpl
