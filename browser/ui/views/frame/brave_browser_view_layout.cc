// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"

#include "brave/browser/ui/views/tabs/features.h"

BraveBrowserViewLayout::~BraveBrowserViewLayout() = default;

void BraveBrowserViewLayout::LayoutSidePanelView(
    views::View* side_panel,
    gfx::Rect& contents_container_bounds) {
  // We don't want to do any special layout for brave's sidebar (which
  // is the parent of chromium's side panel). We simply
  // use flex layout to put it to the side of the content view.
  return;
}

int BraveBrowserViewLayout::LayoutTabStripRegion(int top) {
  if (tabs::features::ShouldShowVerticalTabs())
    return top;

  return BrowserViewLayout::LayoutTabStripRegion(top);
}
