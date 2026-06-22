/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_

#include <utility>

class BrowserWidget;
class BrowserWindowInterface;

// Note that this utils file is directly included by //brave/browser/ui/
// which allows circular dependency between //brave/browser/ui/ and
// //chrome/browser/ui/. So this file should only contain code
// related to vertical tabs and have no choices of including any header
// from //chrome/browser/ui/. If it doesn't have any circular dependency to the
// target, it should be put in VerticalTabController instead.
namespace tabs::utils {

// Returns true when the vertical tab toggle should be enabled.
bool IsVerticalTabToggleEnabled(BrowserWindowInterface* browser);

// Returns window caption buttons' width based on the current platform
std::pair<int, int> GetLeadingTrailingCaptionButtonWidth(
    const BrowserWidget* frame);

}  // namespace tabs::utils

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_UTILS_H_
