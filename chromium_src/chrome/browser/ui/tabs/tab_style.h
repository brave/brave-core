/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STYLE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STYLE_H_

#define GetPinnedWidth virtual GetPinnedWidth
#define GetTabOverlap virtual GetTabOverlap
#define GetSeparatorSize virtual GetSeparatorSize
#define GetSeparatorMargins virtual GetSeparatorMargins
#define GetSeparatorCornerRadius virtual GetSeparatorCornerRadius
#define GetDragHandleExtension virtual GetDragHandleExtension
#define GetTopCornerRadius virtual GetTopCornerRadius
#define GetBottomCornerRadius virtual GetBottomCornerRadius
#define GetContentsInsets virtual GetContentsInsets
#define GetMinimumActiveSplitWidth virtual GetMinimumActiveSplitWidth
#define GetStandardSplitWidth virtual GetStandardSplitWidth

#include "src/chrome/browser/ui/tabs/tab_style.h"  // IWYU pragma: export

#undef GetStandardSplitWidth
#undef GetMinimumActiveSplitWidth
#undef GetPinnedWidth
#undef GetTabOverlap
#undef GetSeparatorSize
#undef GetSeparatorMargins
#undef GetSeparatorCornerRadius
#undef GetDragHandleExtension
#undef GetTopCornerRadius
#undef GetBottomCornerRadius
#undef GetContentsInsets

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STYLE_H_
