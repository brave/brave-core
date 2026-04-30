// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/frame/brave_win_caption_layout.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
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
       tabs::utils::SupportsBraveVerticalTabs(browser))

#define SupportsTouchableTabStrip(browser)                                  \
  SupportsTouchableTabStrip(browser);                                       \
  supports_title =                                                          \
      supports_title_bar ||                                                 \
      (WebUITabStripContainerView::SupportsTouchableTabStrip(browser)       \
           ? WebUITabStripContainerView::SupportsTouchableTabStrip(browser) \
           : tabs::utils::SupportsBraveVerticalTabs(browser))

namespace {

// Translation-unit-local wrapper around `GetLayoutConstant`. Inside Win caption
// button layout, `kTabstripToolbarOverlap` is used to vertically center the
// minimize/maximize/close controls. In compact mode that role wants the
// smaller `tabs::GetHorizontalTabControlsDelta()` value; using the
// tab/toolbar geometry value (which is what `GetBraveLayoutConstant()`
// returns centrally for `kTabstripToolbarOverlap`) leaves the caption
// buttons visibly off-center because their target height is the controls
// row, not the tab strip body. Routing only this TU's `GetLayoutConstant`
// calls through the wrapper keeps the central override intact for every
// other consumer (tab/toolbar geometry, frame view top-area math, etc.) and
// avoids patching upstream. See the matching wrapper in
// `chromium_src/chrome/browser/ui/views/
// frame/horizontal_tab_strip_region_view.cc`.
//
// Standalone web apps with a web-app titlebar (no horizontal tab strip) use
// the same overlap in WebAppFrameToolbar height; the controls delta would
// mismatch (see WebAppBrowserFrameViewWinWindowControlsOverlayTest.
// ContainerHeight).
int GetLayoutConstantForBraveWindowControls(LayoutConstant constant) {
  if (constant == LayoutConstant::kTabstripToolbarOverlap) {
    if (brave::ScopedWinCaptionLayoutUsesGeometryTabstripOverlap::
            GetCurrentWinCaptionGeometryTabstripOverlapDepth() > 0) {
      return ::GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap);
    }
    return tabs::GetHorizontalTabControlsDelta();
  }
  return GetLayoutConstant(constant);
}

}  // namespace

#define GetLayoutConstant GetLayoutConstantForBraveWindowControls
#include <chrome/browser/ui/views/frame/browser_frame_view_win.cc>

#undef GetLayoutConstant
#undef SupportsTouchableTabStrip
#undef SupportsWindowFeature
