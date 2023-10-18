/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_

#include <utility>

class Browser;
class BrowserFrame;

namespace tabs::utils {

// This switch disables vertical tab strip regardless of the pref. This could be
// useful when vertical tab strip causes browser to crash on start up.
constexpr char kDisableVerticalTabsSwitch[] = "disable-vertical-tabs";

// Returns true if the current |browser| might ever support vertical tabs.
bool SupportsVerticalTabs(const Browser* browser);

// Returns true when users chose to use vertical tabs
bool ShouldShowVerticalTabs(const Browser* browser);

// Returns true when we should show window title on window frame when vertical
// tab strip is enabled.
bool ShouldShowWindowTitleForVerticalTabs(const Browser* browser);

// Returns true if we should trigger floating vertical tab strip on mouse over.
bool IsFloatingVerticalTabsEnabled(const Browser* browser);

// Returns window caption buttons' width based on the current platform
std::pair<int, int> GetLeadingTrailingCaptionButtonWidth(
    const BrowserFrame* frame);

}  // namespace tabs::utils

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_
