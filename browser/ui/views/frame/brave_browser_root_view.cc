/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_root_view.h"

#include "brave/browser/ui/views/tabs/features.h"

BraveBrowserRootView::~BraveBrowserRootView() = default;

bool BraveBrowserRootView::OnMouseWheel(const ui::MouseWheelEvent& event) {
  // Bypass BrowserRootView::OnMouseWheel() to avoid tab cycling feature.
  // As vertical tabs are always in a scroll view, we should prefer scrolling
  // to tab cycling.
  if (tabs::features::ShouldShowVerticalTabs())
    return RootView::OnMouseWheel(event);

  return BrowserRootView::OnMouseWheel(event);
}
