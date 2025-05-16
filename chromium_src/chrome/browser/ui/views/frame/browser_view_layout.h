// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_

// Make override-able
#define LayoutTabStripRegion           \
  UnUsed() { return {}; }              \
  friend class BraveBrowserViewLayout; \
  virtual int LayoutTabStripRegion
#define LayoutBookmarkAndInfoBars virtual LayoutBookmarkAndInfoBars
#define LayoutInfoBar virtual LayoutInfoBar
#define LayoutContentsContainerView virtual LayoutContentsContainerView

// Add a new method: NotifyDialogPositionRequiresUpdate(). This is needed for
// split view to update the dialog position when the split view is resized.
#define set_contents_border_widget           \
  set_contents_border_widget_unused();       \
  void NotifyDialogPositionRequiresUpdate(); \
  void set_contents_border_widget

#include "src/chrome/browser/ui/views/frame/browser_view_layout.h"  // IWYU pragma: export

#undef set_contents_border_widget
#undef LayoutContentsContainerView
#undef LayoutInfoBar
#undef LayoutBookmarkAndInfoBars
#undef LayoutTabStripRegion

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_
