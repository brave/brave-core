// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/webui_tab_strip_container_view.h"

// Override invokation of Browser::SupportsWindowFeature in the constructor.
// Note that even though we're overriding the method from BraveBrowser,
// we still need to inline call of it. This is because the BrowserWindow is
// created inside Browser's constructor, so BraveBrowser is not created yet.
// Also, we need to create window_title_ label even when
// Browser::SupportsWindowFeature returns false at the time of creation, as
// users can switch tab orientation later.
#define SupportsWindowFeature(feature)         \
  SupportsWindowFeature(feature) ||            \
      (feature == Browser::FEATURE_TITLEBAR && \
       tabs::utils::SupportsVerticalTabs(browser))

#define SupportsTouchableTabStrip(browser)                             \
  SupportsTouchableTabStrip(browser)                                   \
      ? WebUITabStripContainerView::SupportsTouchableTabStrip(browser) \
      : tabs::utils::SupportsVerticalTabs(browser)

#include "src/chrome/browser/ui/views/frame/browser_frame_view_win.cc"

#undef SupportsTouchableTabStrip
#undef SupportsWindowFeature
