/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_VIEW_H_

#define InstallLoadingIndicator                                       \
  UnUsed() {}                                                         \
                                                                      \
 protected:                                                           \
  void SetLoadingIndicator(                                           \
      std::unique_ptr<PageActionIconLoadingIndicatorView> indicator); \
                                                                      \
 private:                                                             \
  void InstallLoadingIndicator

#include "src/chrome/browser/ui/views/page_action/page_action_icon_view.h"  // IWYU pragma: export

#undef InstallLoadingIndicator

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_VIEW_H_
