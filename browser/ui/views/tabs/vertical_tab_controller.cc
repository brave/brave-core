// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"

#include "base/command_line.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/switches.h"
#include "build/build_config.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"

VerticalTabController::VerticalTabController(BrowserWindowInterface::Type type,
                                             PrefService* prefs)
    : type_(type), prefs_(prefs) {}

VerticalTabController::~VerticalTabController() = default;

bool VerticalTabController::SupportsBraveVerticalTabs() const {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          tabs::switches::kDisableVerticalTabsSwitch)) {
    return false;
  }

  if (tabs::IsVerticalTabsFeatureEnabled()) {
    // In case that Chromium's vertical tabs feature is enabled, we should not
    // show Brave's vertical tabs.
    return false;
  }

  return type_ == BrowserWindowInterface::TYPE_NORMAL;
}

bool VerticalTabController::ShouldShowBraveVerticalTabs() const {
  if (!SupportsBraveVerticalTabs()) {
    return false;
  }

  return prefs_->GetBoolean(brave_tabs::kVerticalTabsEnabled);
}

bool VerticalTabController::ShouldShowWindowTitleForVerticalTabs() const {
  if (!ShouldShowBraveVerticalTabs()) {
    return false;
  }

  return prefs_->GetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow);
}

bool VerticalTabController::IsFloatingVerticalTabsEnabled() const {
  if (!ShouldShowBraveVerticalTabs()) {
    return false;
  }

  if (ShouldHideVerticalTabsCompletelyWhenCollapsed()) {
    // In this case, we should support floating mode regardless of the setting
    // of kVerticalTabsFloatingEnabled.
    return true;
  }

  return prefs_->GetBoolean(brave_tabs::kVerticalTabsFloatingEnabled);
}

bool VerticalTabController::IsVerticalTabOnRight() const {
  return prefs_->GetBoolean(brave_tabs::kVerticalTabsOnRight);
}

bool VerticalTabController::ShouldHideVerticalTabsCompletelyWhenCollapsed()
    const {
  return base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely) &&
         prefs_->GetBoolean(
             brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed);
}
