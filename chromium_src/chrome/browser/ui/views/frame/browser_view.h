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
#include "chrome/browser/ui/views/side_panel/side_panel.h"

#define BrowserViewLayoutDelegateImpl \
  BrowserViewLayoutDelegateImpl;      \
  friend class BraveBrowserView;      \
  void SetNativeWindowPropertyForWidget(views::Widget* widget)
#define BrowserWindow BraveBrowserWindow
#define BrowserViewLayout BraveBrowserViewLayout
#define SidePanel BraveSidePanel
#define BookmarkBarView BraveBookmarkBarView

#define MaybeShowReadingListInSidePanelIPH \
  virtual MaybeShowReadingListInSidePanelIPH

#define UpdateDevToolsForContents virtual UpdateDevToolsForContents
#define GetTabStripVisible virtual GetTabStripVisible

#define GetTabSearchBubbleHost     \
  GetTabSearchBubbleHost_Unused(); \
  virtual TabSearchBubbleHost* GetTabSearchBubbleHost

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
#define GetWidgetForAnchoring                     \
  GetWidgetForAnchoring();                        \
  bool UsesImmersiveFullscreenMode() const;       \
  bool UsesImmersiveFullscreenTabbedMode() const; \
  views::Widget* overlay_widget() {               \
    return overlay_widget_.get();                 \
  }                                               \
  views::View* overlay_view() {                   \
    return overlay_view_.get();                   \
  }                                               \
  views::Widget* tab_overlay_widget() {           \
    return tab_overlay_widget_.get();             \
  }                                               \
  views::View* tab_overlay_view() {               \
    return tab_overlay_view_.get();               \
  }                                               \
  views::View* CreateWinOverlayView
#define contents_separator_                                                \
  contents_separator_ = nullptr;                                           \
  raw_ptr<views::Widget, DanglingUntriaged> overlay_widget_ = nullptr;     \
  raw_ptr<views::Widget, DanglingUntriaged> tab_overlay_widget_ = nullptr; \
  raw_ptr<views::View, DanglingUntriaged> tab_overlay_view_
#endif

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

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
#undef contents_separator_
#undef GetWidgetForAnchoring
#endif

#undef GetTabSearchBubbleHost
#undef GetTabStripVisible
#undef UpdateDevToolsForContents
#undef MaybeShowReadingListInSidePanelIPH
#undef BookmarkBarView
#undef SidePanel
#undef BrowserViewLayout
#undef BrowserWindow
#undef BrowserViewLayoutDelegateImpl

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
