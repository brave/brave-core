// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/browser_commands.h"

#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/views/view.h"

namespace brave {

void FocusLocationBarInFullscreen(Browser* browser) {
  if (!browser || !browser->window() || !browser->window()->IsFullscreen())
    return;

  // Safely access BraveLocationBarView via BrowserView to avoid unsafe casts
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (!browser_view)
    return;

  ToolbarView* toolbar = browser_view->toolbar();
  if (!toolbar)
    return;

  // Use views::AsViewClass for safe type checking
  BraveLocationBarView* brave_location_bar =
      views::AsViewClass<BraveLocationBarView>(toolbar->location_bar());
  if (!brave_location_bar)
    return;

  brave_location_bar->SetTemporaryVisibilityInFullscreen(true);
  brave_location_bar->FocusLocation(false);
}

}  // namespace brave