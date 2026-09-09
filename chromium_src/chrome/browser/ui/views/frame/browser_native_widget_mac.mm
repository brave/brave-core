/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace {

// Upstream sizes the glass effect view to a single region: top chrome with a
// horizontal tab strip, or the tab strip column with vertical tabs. Brave
// extends it over the whole window when vertical tabs are in use, so the glass
// shows through every part of the browser chrome. Covers both the upstream
// vertical tab strip and Brave's own, which are mutually exclusive.
bool BraveShouldUseFullWindowGlassFrame(BrowserView* browser_view) {
  if (!browser_view) {
    return false;
  }
  if (browser_view->ShouldDrawVerticalTabStrip()) {
    return true;
  }
  auto* const controller =
      VerticalTabController::FromBrowser(browser_view->browser());
  return controller && controller->ShouldShowBraveVerticalTabs();
}

}  // namespace

#define BRAVE_BROWSER_NATIVE_WIDGET_MAC_FULL_WINDOW_GLASS_FRAME \
  if (BraveShouldUseFullWindowGlassFrame(browser_view_)) {      \
    return std::nullopt;                                        \
  }

#include <chrome/browser/ui/views/frame/browser_native_widget_mac.mm>

#undef BRAVE_BROWSER_NATIVE_WIDGET_MAC_FULL_WINDOW_GLASS_FRAME
