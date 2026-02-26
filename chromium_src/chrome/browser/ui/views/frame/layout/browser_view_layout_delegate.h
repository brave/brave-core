// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_DELEGATE_H_

// Add methods for BraveBrowserViewLayout to BrowserViewLayoutDelegate.
#define GetExtraInfobarOffset()                                            \
  GetExtraInfobarOffset() const = 0;                                       \
  virtual bool ShouldShowVerticalTabs() const = 0;                         \
  virtual bool IsVerticalTabOnRight() const = 0;                           \
  virtual bool ShouldUseBraveWebViewRoundedCornersForContents() const = 0; \
  virtual int GetRoundedCornersWebViewMargin() const = 0;                  \
  virtual bool IsBookmarkBarOnByPref() const = 0;                          \
  virtual bool IsContentTypeSidePanelVisible() const = 0;                  \
  virtual bool IsFullscreenForBrowser() const = 0;                         \
  virtual bool IsFullscreenForTab() const = 0;                             \
  virtual bool IsFullscreen()

#include <chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h>  // IWYU pragma: export

#undef GetExtraInfobarOffset

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_DELEGATE_H_
