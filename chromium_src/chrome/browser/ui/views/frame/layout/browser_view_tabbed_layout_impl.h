// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_

// Make GetTopSeparatorType virtual to override it in
// BraveBrowserViewTabbedLayoutImpl.
#define GetTopSeparatorType()                    \
  GetTopSeparatorType_Unused();                  \
  friend class BraveBrowserViewTabbedLayoutImpl; \
  virtual TopSeparatorType GetTopSeparatorType()

#include <chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h>  // IWYU pragma: export

#undef GetTopSeparatorType

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
