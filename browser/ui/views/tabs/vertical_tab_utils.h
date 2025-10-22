/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_

#include <utility>

class Browser;
class BrowserWidget;

namespace tabs::utils {

// Returns true if the current |browser| might ever support vertical tabs.
bool SupportsVerticalTabs(const Browser* browser);

// Returns true when users chose to use vertical tabs
bool ShouldShowVerticalTabs(const Browser* browser);

// Returns true when we should show window title on window frame when vertical
// tab strip is enabled.
bool ShouldShowWindowTitleForVerticalTabs(const Browser* browser);

// Returns true if we should trigger floating vertical tab strip on mouse over.
bool IsFloatingVerticalTabsEnabled(const Browser* browser);

bool IsVerticalTabOnRight(const Browser* browser);

// Returns true if we should hide vertical tab completely when collapsed. In
// this case vertical tab strip will be almost a few pixels wide vertical bar
// when collapsed. And will expand to show tab icons when mouse is over the bar
// regardless of the setting of kVerticalTabsFloatingEnabled.
bool ShouldHideVerticalTabsCompletelyWhenCollapsed(const Browser* browser);

// Returns window caption buttons' width based on the current platform
std::pair<int, int> GetLeadingTrailingCaptionButtonWidth(
    const BrowserWidget* frame);

}  // namespace tabs::utils

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_
