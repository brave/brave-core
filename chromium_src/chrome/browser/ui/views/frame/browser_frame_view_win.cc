/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/features.h"

// The tab region view maintains its own padding; the frame view does not need
// to reserve an extra top margin for it.
#define BRAVE_BROWSER_FRAME_VIEW_WIN_TOP_AREA_HEIGHT   \
  if (tabs::features::HorizontalTabsUpdateEnabled()) { \
    return top;                                        \
  }

#include "src/chrome/browser/ui/views/frame/browser_frame_view_win.cc"  // IWYU pragma: export

#undef BRAVE_BROWSER_FRAME_VIEW_WIN_TOP_AREA_HEIGHT
