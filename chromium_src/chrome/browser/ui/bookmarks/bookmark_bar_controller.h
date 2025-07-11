/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_BAR_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_BAR_CONTROLLER_H_

#include "src/chrome/browser/ui/bookmarks/bookmark_bar_controller.h"  // IWYU pragma: export

// Exposing this function from the anonymous namespace in the original file
// so that it can be used in other parts of the codebase.
bool IsShowingNTP_ChromiumImpl(content::WebContents* web_contents);

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_BAR_CONTROLLER_H_
