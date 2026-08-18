/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_HORIZONTAL_TAB_STYLE_VIEWS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_HORIZONTAL_TAB_STYLE_VIEWS_H_

#include <chrome/browser/ui/views/tabs/horizontal_tab_style_views.h>  // IWYU pragma: export

#include <memory>

// Constructs a BraveVerticalTabStyle instance.
std::unique_ptr<HorizontalTabStyleViews> CreateBraveVerticalTabStyle(
    std::unique_ptr<TabStyleViewDelegate> delegate);

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_HORIZONTAL_TAB_STYLE_VIEWS_H_
