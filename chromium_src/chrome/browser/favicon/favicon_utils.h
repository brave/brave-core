// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FAVICON_FAVICON_UTILS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FAVICON_FAVICON_UTILS_H_

#define ShouldThemifyFaviconForEntry                                          \
  ShouldThemifyFaviconForEntry_ChromiumImpl(content::NavigationEntry* entry); \
  bool ShouldThemifyFaviconForEntry
#include "src/chrome/browser/favicon/favicon_utils.h"  // IWYU pragma: export
#undef ShouldThemifyFaviconForEntry

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FAVICON_FAVICON_UTILS_H_
