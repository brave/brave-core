/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_SHARED_PINNED_TAB_UTILS_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_SHARED_PINNED_TAB_UTILS_H_

class BrowserWindowInterface;

namespace brave_tabs {

// Called after a tab of `browser` closes. When only shared pinned tabs remain
// and the profile has another window, closes `browser` without asking, as its
// shared pinned tabs also live in the other window.
void MaybeCloseWindowWithOnlySharedPinnedTabs(BrowserWindowInterface* browser);

}  // namespace brave_tabs

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_SHARED_PINNED_TAB_UTILS_H_
