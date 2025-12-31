// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/frame/mock_immersive_mode_controller.h"

// Provides default implementations for all methods in
// MockBrowserViewLayoutDelegate.
#define GetExtraInfobarOffset()                                          \
  GetExtraInfobarOffset_Unused() {                                       \
    return 0;                                                            \
  }                                                                      \
  bool ShouldShowVerticalTabs() const override {                         \
    return false;                                                        \
  }                                                                      \
  bool IsVerticalTabOnRight() const override {                           \
    return false;                                                        \
  }                                                                      \
  bool ShouldUseBraveWebViewRoundedCornersForContents() const override { \
    return false;                                                        \
  }                                                                      \
  int GetRoundedCornersWebViewMargin() override {                        \
    return 0;                                                            \
  }                                                                      \
  bool IsBookmarkBarOnByPref() const override {                          \
    return false;                                                        \
  }                                                                      \
  bool IsContentTypeSidePanelVisible() override {                        \
    return false;                                                        \
  }                                                                      \
  bool IsFullscreenForBrowser() override {                               \
    return false;                                                        \
  }                                                                      \
  bool IsFullscreenForTab() override {                                   \
    return false;                                                        \
  }                                                                      \
  bool IsFullscreenForTab() const override {                             \
    return false;                                                        \
  }                                                                      \
  bool IsFullscreen() const override {                                   \
    return false;                                                        \
  }                                                                      \
  int GetExtraInfobarOffset()

#include <chrome/browser/ui/views/frame/layout/browser_view_layout_unittest.cc>

#undef GetExtraInfobarOffset
