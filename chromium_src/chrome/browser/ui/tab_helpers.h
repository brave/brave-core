// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_HELPERS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_HELPERS_H_

class PeekContents;

// Peek shows a page over the active tab and can promote it to a tab, so it
// needs the full set of tab helpers.
#define AttachTabHelpers       \
  AttachTabHelpers_Unused();   \
  friend class ::PeekContents; \
  static void AttachTabHelpers

#include <chrome/browser/ui/tab_helpers.h>  // IWYU pragma: export

#undef AttachTabHelpers

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_HELPERS_H_
