// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
// button layout, `kTabstripToolbarOverlap` is used to compute the caption
// button container height. This must use the geometry overlap value (what
// `GetBraveLayoutConstant()` returns for `kTabstripToolbarOverlap`, e.g. 1 in
// default mode and 8 in compact mode) — always a positive value that correctly
// sizes the container. Using `tabs::GetHorizontalTabControlsDelta()` here was
// wrong: it is negative in both default (-4) and compact (-5) mode and caused
// the container height to be miscalculated
// (https://github.com/brave/brave-browser/issues/55406).
// `::GetLayoutConstant` bypasses the `#define` redirection and calls the
// Brave-overridden global directly, keeping the central override intact for
// every other consumer (tab/toolbar geometry, frame view top-area math, etc.)
// and avoiding patching upstream.
// Note: `horizontal_tab_strip_region_view.cc` has a similar wrapper that
// intentionally DOES use `GetHorizontalTabControlsDelta()` for centering
// tab-strip control buttons (new tab, tab search, etc.) — a different role.
int GetLayoutConstantForBraveWindowControls(LayoutConstant constant) {
  if (constant == LayoutConstant::kTabstripToolbarOverlap) {
      return ::GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap);
  }
  return GetLayoutConstant(constant);
}

}  // namespace

#define GetLayoutConstant GetLayoutConstantForBraveWindowControls
#include <chrome/browser/ui/views/frame/browser_frame_view_win.cc>

#undef GetLayoutConstant
#undef SupportsTouchableTabStrip
#undef SupportsWindowFeature
