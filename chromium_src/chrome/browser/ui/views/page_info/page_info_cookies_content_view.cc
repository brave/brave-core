/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_info/page_info_cookies_content_view.h"

#include "chrome/browser/ui/views/page_info/page_info_main_view.h"

#define SetCookieInfo SetCookieInfo_ChromiumImpl
#include "src/chrome/browser/ui/views/page_info/page_info_cookies_content_view.cc"
#undef SetCookieInfo

void PageInfoCookiesContentView::SetCookieInfo(
    const CookiesNewInfo& cookie_info) {
  SetCookieInfo_ChromiumImpl(cookie_info);

  // Remove cookies text and link to settings.
  RemoveChildView(children()[0]);

  // Hide 3P cookies toggle if shown.
  if (blocking_third_party_cookies_row_) {
    blocking_third_party_cookies_row_->SetVisible(false);
  }

  PreferredSizeChanged();
}
