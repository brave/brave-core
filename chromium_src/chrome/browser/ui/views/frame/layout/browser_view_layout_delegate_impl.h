// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_DELEGATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_DELEGATE_IMPL_H_

#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"

#define GetExtraInfobarOffset()                                         \
  GetExtraInfobarOffset() const override;                               \
  bool ShouldShowVerticalTabs() const override;                         \
  bool IsVerticalTabOnRight() const override;                           \
  bool ShouldUseBraveWebViewRoundedCornersForContents() const override; \
  int GetRoundedCornersWebViewMargin() override;                        \
  bool IsBookmarkBarOnByPref() const override;                          \
  bool IsContentTypeSidePanelVisible() override;                        \
  bool IsFullscreenForBrowser() override;                               \
  bool IsFullscreenForTab() override;                                   \
  bool IsFullscreenForTab() const override;                             \
  bool IsFullscreen()

#include <chrome/browser/ui/views/frame/layout/browser_view_layout_delegate_impl.h>  // IWYU pragma: export

#undef GetExtraInfobarOffset

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_DELEGATE_IMPL_H_
