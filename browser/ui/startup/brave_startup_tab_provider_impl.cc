/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"

#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/startup/startup_tab.h"
#include "chrome/common/webui_url_constants.h"

StartupTabs BraveStartupTabProviderImpl::GetDistributionFirstRunTabs(
    StartupBrowserCreator* browser_creator) const {
  StartupTabs tabs =
      StartupTabProviderImpl::GetDistributionFirstRunTabs(browser_creator);
  if (first_run::IsChromeFirstRun()) {
    tabs.emplace_back(GURL(chrome::kChromeUIWelcomeURL));
  }
  return tabs;
}
