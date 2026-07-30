/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_EDITOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_EDITOR_H_

#include <optional>

// Allow callers to override the dialog window title resource id (e.g. use
// "Bookmark selected tabs" instead of Chromium's "Bookmark all tabs").
#define bookmark_data \
  bookmark_data;      \
  std::optional<int> window_title_id

#define GetWindowTitleId                 \
  GetWindowTitleId_ChromiumImpl() const; \
  int GetWindowTitleId

#include <chrome/browser/ui/bookmarks/bookmark_editor.h>  // IWYU pragma: export

#undef GetWindowTitleId
#undef bookmark_data

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_EDITOR_H_
