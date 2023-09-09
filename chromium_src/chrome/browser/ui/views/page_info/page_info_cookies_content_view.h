/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_COOKIES_CONTENT_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_COOKIES_CONTENT_VIEW_H_

#include "components/page_info/page_info_ui.h"

#define SetCookieInfo                                            \
  SetCookieInfo_ChromiumImpl(const CookiesNewInfo& cookie_info); \
  void SetCookieInfo

#include "src/chrome/browser/ui/views/page_info/page_info_cookies_content_view.h"  // IWYU pragma: export
#undef SetCookieInfo

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_COOKIES_CONTENT_VIEW_H_
