// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

BraveLocationBarView::BraveLocationBarView(Browser* browser,
                                           Profile* profile,
                                           CommandUpdater* command_updater,
                                           Delegate* delegate,
                                           bool is_popup_mode)
    : LocationBarView(browser, profile, command_updater, delegate, is_popup_mode) {}

BraveLocationBarView::~BraveLocationBarView() = default;

void BraveLocationBarView::OnOmniboxBlurred() {
  LocationBarView::OnOmniboxBlurred();
  
  if (is_temporarily_visible_in_fullscreen_) {
    SetTemporaryVisibilityInFullscreen(false);
  }
}

void BraveLocationBarView::SetTemporaryVisibilityInFullscreen(bool visible) {
  Browser* browser = GetBrowser();
  if (!browser || !browser->window() || !browser->window()->IsFullscreen())
    return;

  is_temporarily_visible_in_fullscreen_ = visible;

  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (!browser_view)
    return;

  ToolbarView* toolbar = browser_view->toolbar();
  if (!toolbar)
    return;

  if (visible) {
    toolbar->SetVisible(true);
    toolbar->Layout();
  } else {
    toolbar->SetVisible(false);
    is_temporarily_visible_in_fullscreen_ = false;
  }

  browser_view->Layout();
}