/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

#define BrowserViewLayoutDelegateImpl \
  BrowserViewLayoutDelegateImpl;      \
  friend class BraveBrowserView;      \
  void SetNativeWindowPropertyForWidget(views::Widget* widget)
#define BrowserWindow BraveBrowserWindow
#define BrowserViewLayout BraveBrowserViewLayout
#define SidePanel BraveSidePanel

#define MaybeShowReadingListInSidePanelIPH \
  virtual MaybeShowReadingListInSidePanelIPH

#define UpdateDevToolsForContents virtual UpdateDevToolsForContents
#define GetTabStripVisible virtual GetTabStripVisible

#define GetTabSearchBubbleHost     \
  GetTabSearchBubbleHost_Unused(); \
  virtual TabSearchBubbleHost* GetTabSearchBubbleHost

#if BUILDFLAG(IS_WIN)
#define GetSupportsTitle virtual GetSupportsTitle

// On Windows <winuser.h> defines LoadAccelerators
#pragma push_macro("LoadAccelerators")
#undef LoadAccelerators
#endif
#define LoadAccelerators virtual LoadAccelerators

#include "src/chrome/browser/ui/views/frame/browser_view.h"  // IWYU pragma: export

#undef LoadAccelerators
#if BUILDFLAG(IS_WIN)
#pragma pop_macro("LoadAccelerators")
#undef GetSupportsTitle
#endif

#undef GetTabSearchBubbleHost
#undef GetTabStripVisible
#undef UpdateDevToolsForContents
#undef MaybeShowReadingListInSidePanelIPH
#undef SidePanel
#undef BrowserViewLayout
#undef BrowserWindow
#undef BrowserViewLayoutDelegateImpl

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
