/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_IMPL_OLD_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_IMPL_OLD_H_

#include "chrome/browser/ui/views/frame/layout/browser_view_layout.h"

// Make BrowserViewLayoutImplOld override-able
#define LayoutTabStripRegion           \
  UnUsed();                            \
  friend class BraveBrowserViewLayout; \
  virtual void LayoutTabStripRegion

#define IsImmersiveModeEnabledWithoutToolbar \
  virtual IsImmersiveModeEnabledWithoutToolbar
#define LayoutBookmarkBar virtual LayoutBookmarkBar
#define LayoutInfoBar virtual LayoutInfoBar
#define LayoutContentsContainerView virtual LayoutContentsContainerView

#include <chrome/browser/ui/views/frame/layout/browser_view_layout_impl_old.h>  // IWYU pragma: export

#undef LayoutContentsContainerView
#undef LayoutInfoBar
#undef LayoutBookmarkBar
#undef IsImmersiveModeEnabledWithoutToolbar
#undef LayoutTabStripRegion

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_IMPL_OLD_H_
