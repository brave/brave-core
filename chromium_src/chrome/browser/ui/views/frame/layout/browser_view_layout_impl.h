// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_IMPL_H_

#include "ui/views/layout/proposed_layout.h"

// Add non-const version of PropsedLayout::GetLayoutFor()
#define GetLayoutFor(...)                    \
  GetLayoutFor_Unused();                     \
  ProposedLayout* GetLayoutFor(__VA_ARGS__); \
  const ProposedLayout* GetLayoutFor(__VA_ARGS__)

#include <chrome/browser/ui/views/frame/layout/browser_view_layout_impl.h>  // IWYU pragma: export

#undef GetLayoutFor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_LAYOUT_IMPL_H_
