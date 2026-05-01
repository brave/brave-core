/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/command_line.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/switches.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"


namespace tabs::utils {

bool SupportsBraveVerticalTabs(const BrowserWindowInterface* browser) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableVerticalTabsSwitch)) {
    return false;
  }

  if (tabs::IsVerticalTabsFeatureEnabled()) {
    // In case that Chromium's vertical tabs feature is enabled, we should not
    // show Brave's vertical tabs.
    return false;
  }

  if (!browser) {
    // During unit tests, |browser| can be null.
    CHECK_IS_TEST();
    return false;
  }

  return browser->GetType() == BrowserWindowInterface::TYPE_NORMAL;
}

bool ShouldShowBraveVerticalTabs(const BrowserWindowInterface* browser) {
  if (!SupportsBraveVerticalTabs(browser)) {
    return false;
  }

  return browser->GetProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsEnabled);
}

bool ShouldShowWindowTitleForVerticalTabs(
    const BrowserWindowInterface* browser) {
  if (!ShouldShowBraveVerticalTabs(browser)) {
    return false;
  }

  return browser->GetProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsShowTitleOnWindow);
}

bool IsFloatingVerticalTabsEnabled(const BrowserWindowInterface* browser) {
  if (!ShouldShowBraveVerticalTabs(browser)) {
    return false;
  }

  if (ShouldHideVerticalTabsCompletelyWhenCollapsed(browser)) {
    // In this case, we should support floating mode regardless of the setting
    // of kVerticalTabsFloatingEnabled.
    return true;
  }

  return browser->GetProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsFloatingEnabled);
}

bool IsVerticalTabOnRight(const BrowserWindowInterface* browser) {
  return browser->GetProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsOnRight);
}

bool ShouldHideVerticalTabsCompletelyWhenCollapsed(
    const BrowserWindowInterface* browser) {
  return base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely) &&
         browser->GetProfile()->GetPrefs()->GetBoolean(
             brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed);
}

bool IsVerticalTabToggleEnabled(BrowserWindowInterface* browser) {
#if BUILDFLAG(IS_MAC)
  if (!browser) {
    return true;
  }
  return browser->GetFeatures().browser_command_controller()->IsCommandEnabled(
      IDC_TOGGLE_VERTICAL_TABS);
#else
  return true;
#endif
}

}  // namespace tabs::utils
