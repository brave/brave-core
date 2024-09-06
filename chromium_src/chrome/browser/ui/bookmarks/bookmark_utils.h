// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_UTILS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_UTILS_H_

#include "chrome/browser/ui/bookmarks/bookmark_utils.h"

// This replace is for changing the chrome:// to brave:// scheme displayed in
// Bookmarks Add page
#define FormatBookmarkURLForDisplay                          \
  FormatBookmarkURLForDisplay_ChromiumImpl(const GURL& url); \
  std::u16string FormatBookmarkURLForDisplay
#include "../../../../../../chrome/browser/ui/bookmarks/bookmark_utils.h"
#undef FormatBookmarkURLForDisplay
#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_UTILS_H_
