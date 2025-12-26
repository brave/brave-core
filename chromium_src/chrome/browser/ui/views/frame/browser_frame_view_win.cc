// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/webui_tab_strip_container_view.h"

// Override invokation of Browser::SupportsWindowFeature() and
// WebUITabStripContainerView::SupportsTouchableTabStrip() in the constructor in
// order to create the `window_title_` label when the browser supports the
// vertical tab strip. Note that even though we're overriding the method from
// BraveBrowser::SupportsWindowFeature(), we still need to inline call of it.
// This is because the BrowserWindow is created inside Browser's constructor, so
// BraveBrowser is not created yet. Also, we need to create `window_title_`
// label even when Browser::SupportsWindowFeature(kFeatureTitleBar) returns
// false at the time of creation, as users can switch tab orientation or title
// bar visibility by changing preferences.
#define SupportsWindowFeature(feature)                        \
  SupportsWindowFeature(feature) ||                           \
      (feature == Browser::WindowFeature::kFeatureTitleBar && \
       tabs::utils::SupportsVerticalTabs(browser))

#define SupportsTouchableTabStrip(browser)                                  \
  SupportsTouchableTabStrip(browser);                                       \
  supports_title =                                                          \
      supports_title_bar ||                                                 \
      (WebUITabStripContainerView::SupportsTouchableTabStrip(browser)       \
           ? WebUITabStripContainerView::SupportsTouchableTabStrip(browser) \
           : tabs::utils::SupportsVerticalTabs(browser))

#include <chrome/browser/ui/views/frame/browser_frame_view_win.cc>

#undef SupportsTouchableTabStrip
#undef SupportsWindowFeature
