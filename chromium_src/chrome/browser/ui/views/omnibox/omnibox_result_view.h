/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_VIEW_H_

#define SetMatch                       \
  UnUsed() {}                          \
  friend class BraveOmniboxResultView; \
  virtual void SetMatch
#define OnSelectionStateChanged virtual OnSelectionStateChanged
#define GetIcon virtual GetIcon

#include "src/chrome/browser/ui/views/omnibox/omnibox_result_view.h"  // IWYU pragma: export

#undef GetIcon
#undef OnSelectionStateChanged
#undef SetMatch

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_VIEW_H_
