/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_ROW_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_ROW_VIEW_H_

#include "ui/views/view.h"

#define GetInsets           \
  GetInsets_UnUsed() const; \
  gfx::Insets GetInsets

#include "src/chrome/browser/ui/views/omnibox/omnibox_row_view.h"  // IWYU pragma: export

#undef GetInsets

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_VIEW_H_
