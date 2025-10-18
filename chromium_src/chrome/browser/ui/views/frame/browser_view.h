/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/views/bookmarks/brave_bookmark_bar_view.h"
#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "build/build_config.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

#define BrowserViewLayoutDelegateImplNew \
  BrowserViewLayoutDelegateImplNew;      \
  friend class BraveBrowserView;         \
  void SetNativeWindowPropertyForWidget(views::Widget* widget)
#define BrowserWindow BraveBrowserWindow
#define BrowserViewLayout BraveBrowserViewLayout
#define SidePanel BraveSidePanel
#define BookmarkBarView BraveBookmarkBarView

#define MaybeShowReadingListInSidePanelIPH \
  virtual MaybeShowReadingListInSidePanelIPH

#define MaybeUpdateDevtools virtual MaybeUpdateDevtools
#define GetTabStripVisible virtual GetTabStripVisible

#define GetTabSearchBubbleHost     \
  GetTabSearchBubbleHost_Unused(); \
  virtual TabSearchBubbleHost* GetTabSearchBubbleHost

#define UpdateExclusiveAccessBubble                            \
  UpdateExclusiveAccessBubble_ChromiumImpl(                    \
      const ExclusiveAccessBubbleParams& params,               \
      ExclusiveAccessBubbleHideCallback first_hide_callback);  \
  void UpdateExclusiveAccessBubble

#if BUILDFLAG(IS_WIN)
// On Windows <winuser.h> defines LoadAccelerators
// Using push_macro seems to be causing #undef not to work in Chromium 125.
// Unclear what causes this.
// #pragma push_macro("LoadAccelerators")
#undef LoadAccelerators
#endif
#define LoadAccelerators virtual LoadAccelerators
#define ShowSplitView virtual ShowSplitView
#define HideSplitView virtual HideSplitView

#include <chrome/browser/ui/views/frame/browser_view.h>  // IWYU pragma: export

#undef HideSplitView
#undef ShowSplitView
#undef LoadAccelerators
#if BUILDFLAG(IS_WIN)
// #pragma pop_macro("LoadAccelerators")
#endif

#undef UpdateExclusiveAccessBubble
#undef GetTabSearchBubbleHost
#undef GetTabStripVisible
#undef MaybeUpdateDevtools
#undef MaybeShowReadingListInSidePanelIPH
#undef BookmarkBarView
#undef SidePanel
#undef BrowserViewLayout
#undef BrowserWindow
#undef BrowserViewLayoutDelegateImplNew

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
