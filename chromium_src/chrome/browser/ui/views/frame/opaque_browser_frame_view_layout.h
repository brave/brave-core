/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_LAYOUT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_LAYOUT_H_

#define SetBoundsForButton                  \
  SetBoundsForButton_Unused();              \
  friend class BrowserFrameViewLayoutLinux; \
  virtual void SetBoundsForButton

#include "src/chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"  // IWYU pragma: export

#undef SetBoundsForButton

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_LAYOUT_H_
