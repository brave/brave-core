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
#include "chrome/browser/ui/views/side_search/side_search_browser_controller.h"

#define BrowserViewLayoutDelegateImpl \
  BrowserViewLayoutDelegateImpl;      \
  friend class BraveBrowserView
#define BrowserWindow BraveBrowserWindow
#define BrowserViewLayout BraveBrowserViewLayout
#define SidePanel BraveSidePanel
#define GetContentsLayoutManager                                 \
  GetContentsLayoutManager_Unused();                             \
  static const char* GetBrowserViewKeyForNativeWindowProperty(); \
  virtual ContentsLayoutManager* GetContentsLayoutManager

#define MaybeShowReadingListInSidePanelIPH \
  virtual MaybeShowReadingListInSidePanelIPH

#define GetTabStripVisible virtual GetTabStripVisible
#define BrowserViewLayout BraveBrowserViewLayout

#if BUILDFLAG(IS_WIN)
#define GetSupportsTitle virtual GetSupportsTitle
#endif

#include "src/chrome/browser/ui/views/frame/browser_view.h"

#if BUILDFLAG(IS_WIN)
#undef GetSupportsTitle
#endif

#undef BrowserViewLayout
#undef GetTabStripVisible
#undef BrowserViewLayoutDelegateImpl
#undef BrowserWindow
#undef MaybeShowReadingListInSidePanelIPH
#undef BrowserViewLayout
#undef SidePanel
#undef GetContentsLayoutManager

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
