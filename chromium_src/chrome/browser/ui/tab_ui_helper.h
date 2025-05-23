/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_UI_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_UI_HELPER_H_

#include "content/public/browser/web_contents_user_data.h"

#define GetTitle()                               \
  GetTitle_ChromiumImpl() const;                 \
                                                 \
 private:                                        \
  mutable std::u16string cached_container_name_; \
                                                 \
 public:                                         \
  std::u16string GetTitle()

#include "src/chrome/browser/ui/tab_ui_helper.h"  // IWYU pragma: export

#undef GetTitle

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TAB_UI_HELPER_H_
