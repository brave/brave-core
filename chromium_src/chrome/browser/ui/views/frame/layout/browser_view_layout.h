/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_H_

class SidebarContainerView;

// Make private members of BrowserViewLayout accessible
// TODO(https://github.com/brave/brave-browser/issues/50488): There should be no
// need to access these private members directly once the const-correctness
// around these classes is resolved.
#define ShouldDisplayVerticalTabs      \
  UnUsed();                            \
  friend class BraveBrowserViewLayout; \
  bool ShouldDisplayVerticalTabs

// Add a new method: NotifyDialogPositionRequiresUpdate(). This is needed for
// split view to update the dialog position when the split view is resized.
#define set_webui_tab_strip                  \
  set_webui_tab_strip_unused();              \
  void NotifyDialogPositionRequiresUpdate(); \
  void set_webui_tab_strip

// Add new members to BrowserViewLayoutViews for Brave specific layout changes.
#define top_container_separator                              \
  top_container_separator;                                   \
  raw_ptr<views::View> contents_background = nullptr;        \
  raw_ptr<views::View> vertical_tab_strip_host = nullptr;    \
  raw_ptr<SidebarContainerView> sidebar_container = nullptr; \
  raw_ptr<views::View> sidebar_separator

// Add setters for the new members to BrowserViewLayout.
#define set_side_panel_animation_content                                   \
  set_contents_background(views::View* contents_background) {              \
    views_.contents_background = contents_background;                      \
  }                                                                        \
  void set_vertical_tab_strip_host(views::View* vertical_tab_strip_host) { \
    views_.vertical_tab_strip_host = vertical_tab_strip_host;              \
  }                                                                        \
  void set_sidebar_container(SidebarContainerView* sidebar_container) {    \
    views_.sidebar_container = sidebar_container;                          \
  }                                                                        \
  void set_sidebar_separator(views::View* sidebar_separator) {             \
    views_.sidebar_separator = sidebar_separator;                          \
  }                                                                        \
  void set_side_panel_animation_content

#include <chrome/browser/ui/views/frame/layout/browser_view_layout.h>  // IWYU pragma: export

#undef set_side_panel_animation_content
#undef top_container_separator
#undef ShouldDisplayVerticalTabs
#undef set_webui_tab_strip

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_H_
