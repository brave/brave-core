// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_

// Make BrowserViewLayoutImplOld override-able
#define LayoutTabStripRegion           \
  UnUsed();                            \
  friend class BraveBrowserViewLayout; \
  virtual void LayoutTabStripRegion

// Make private members of BrowserViewLayout accessible
// TODO(https://github.com/brave/brave-browser/issues/50488): There should be no
// need to access these private members directly once the const-correctness
// around these classes is resolved.
#define ShouldDisplayVerticalTabs      \
  UnUsed();                            \
  friend class BraveBrowserViewLayout; \
  bool ShouldDisplayVerticalTabs

#define IsImmersiveModeEnabledWithoutToolbar \
  virtual IsImmersiveModeEnabledWithoutToolbar
#define LayoutBookmarkBar virtual LayoutBookmarkBar
#define LayoutInfoBar virtual LayoutInfoBar
#define LayoutContentsContainerView virtual LayoutContentsContainerView

// Add a new method: NotifyDialogPositionRequiresUpdate(). This is needed for
// split view to update the dialog position when the split view is resized.
#define set_webui_tab_strip                  \
  set_webui_tab_strip_unused();              \
  void NotifyDialogPositionRequiresUpdate(); \
  void set_webui_tab_strip

#include <chrome/browser/ui/views/frame/browser_view_layout.h>  // IWYU pragma: export

#undef set_webui_tab_strip
#undef LayoutContentsContainerView
#undef LayoutInfoBar
#undef LayoutBookmarkBar
#undef IsImmersiveModeEnabledWithoutToolbar
#undef LayoutTabStripRegion
#undef ShouldDisplayVerticalTabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_
