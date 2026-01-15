// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_LAYOUT_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_LAYOUT_DELEGATE_IMPL_H_

#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate_impl.h"

class BraveBrowserViewLayoutDelegateImpl
    : public BrowserViewLayoutDelegateImpl {
 public:
  using BrowserViewLayoutDelegateImpl::BrowserViewLayoutDelegateImpl;
  ~BraveBrowserViewLayoutDelegateImpl() override;

  // Delegate methods for BraveBrowserViewLayout
  bool ShouldShowVerticalTabs() const;
  bool IsVerticalTabOnRight() const;
  bool ShouldUseBraveWebViewRoundedCornersForContents() const;
  int GetRoundedCornersWebViewMargin();
  bool IsBookmarkBarOnByPref() const;
  bool IsContentTypeSidePanelVisible();
  bool IsFullscreenForBrowser();
  bool IsFullscreenForTab();
  bool IsFullscreenForTab() const;
  bool IsFullscreen() const;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_LAYOUT_DELEGATE_IMPL_H_
