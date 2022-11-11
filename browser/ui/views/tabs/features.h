/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_FEATURES_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_FEATURES_H_

#include <utility>

#include "base/compiler_specific.h"

namespace base {
struct LOGICALLY_CONST Feature;
}  // namespace base

class Browser;
class BrowserFrame;

namespace tabs {
namespace features {

extern const base::Feature kBraveVerticalTabs;

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

}  // namespace features
}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_FEATURES_H_
