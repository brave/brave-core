/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

// Currently, `NewTabButton::kButtonSize` is used to calculate tab strip button
// borders. Since the size of buttons varies depending upon the tabs update
// feature flag, replace the `kButtonSize` identifier with a call to the static
// `BraveNewTabButton::GetButtonSize` function.
#define kButtonSize GetButtonSize()

namespace {

// Tab-strip-region-local override of `GetLayoutConstant`. Inside the upstream
// `UpdateButtonBorders()` body, `kTabstripToolbarOverlap` is used to vertically
// center tab-strip *control* buttons (new tab button, combo button, tab search
// button, …). In compact mode that role wants the smaller
// `tabs::GetHorizontalTabControlsDelta()` value, while every other consumer of
// `kTabstripToolbarOverlap` (tab/toolbar geometry, frame views, etc.) wants
// the larger value supplied by the central `GetBraveLayoutConstant()`
// override in `chromium_src/chrome/browser/ui/layout_constants.cc`. Routing
// just this translation unit's `GetLayoutConstant` calls through the wrapper
// avoids patching upstream and mirrors the same pattern used in
// `chromium_src/chrome/browser/ui/views/frame/browser_frame_view_win.cc`.
int GetLayoutConstantForBraveHorizontalTabStripRegion(LayoutConstant constant) {
  if (constant == LayoutConstant::kTabstripToolbarOverlap) {
    return tabs::GetHorizontalTabControlsDelta();
  }
  return GetLayoutConstant(constant);
}

}  // namespace

#define BrowserTabStripController BraveBrowserTabStripController
#define GetLayoutConstant GetLayoutConstantForBraveHorizontalTabStripRegion
#define NewTabButton BraveNewTabButton
#define TabHoverCardController BraveTabHoverCardController
#define TabSearchButton BraveTabSearchButton
#include <chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.cc>
#undef TabSearchButton
#undef TabHoverCardController
#undef NewTabButton
#undef GetLayoutConstant
#undef BrowserTabStripController
#undef kButtonSize
