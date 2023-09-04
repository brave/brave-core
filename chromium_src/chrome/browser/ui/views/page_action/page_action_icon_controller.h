/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_CONTROLLER_H_

#define UpdateAll                                                   \
  UpdateAll();                                                      \
                                                                    \
 private:                                                           \
  raw_ptr<PageActionIconView> playlist_action_icon_view_ = nullptr; \
                                                                    \
 public:                                                            \
  PageActionIconView* GetPlaylistActionIconView

#include "src/chrome/browser/ui/views/page_action/page_action_icon_controller.h"  // IWYU pragma: export
#undef UpdateAll

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_CONTROLLER_H_
