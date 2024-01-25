/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_READING_LIST_READING_LIST_PAGE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_READING_LIST_READING_LIST_PAGE_HANDLER_H_

#define UpdateCurrentPageActionButton       \
  UpdateCurrentPageActionButton_UnUsed() {} \
  friend class BraveReadingListPageHandler; \
  void UpdateCurrentPageActionButton

#include "src/chrome/browser/ui/webui/side_panel/reading_list/reading_list_page_handler.h"  // IWYU pragma: export

#undef UpdateCurrentPageActionButton

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_READING_LIST_READING_LIST_PAGE_HANDLER_H_
