/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_COOKIES_CONTENT_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_COOKIES_CONTENT_VIEW_H_

#include "components/page_info/page_info_ui.h"

class PageInfoCookiesContentView;
using PageInfoCookiesContentView_BraveImpl = PageInfoCookiesContentView;

#define InitCookiesDialogButton                \
  Unused();                                    \
  friend PageInfoCookiesContentView_BraveImpl; \
  void InitCookiesDialogButton

#define SetThirdPartyCookiesInfo virtual SetThirdPartyCookiesInfo

#define PageInfoCookiesContentView PageInfoCookiesContentView_ChromiumImpl
#include "src/chrome/browser/ui/views/page_info/page_info_cookies_content_view.h"  // IWYU pragma: export
#undef PageInfoCookiesContentView
#undef SetThirdPartyCookiesInfo
#undef InitCookiesDialogButton

// This class is used to provide the necessary overrides to hide from view
// the third-party cookies section in the Page Info UI for Brave. This means
// intercepting `SetCookieInfo` and disabling the controls at that stage, and
// also intercepting `SetThirdPartyCookiesInfo` to ensure that the third-party
// cookie is processed as `kHidden`.
class PageInfoCookiesContentView
    : public PageInfoCookiesContentView_ChromiumImpl {
 public:
  using PageInfoCookiesContentView_ChromiumImpl::
      PageInfoCookiesContentView_ChromiumImpl;

  // Overrides from PageInfoCookiesContentView_ChromiumImpl.
  void SetCookieInfo(const CookiesNewInfo& cookie_info) override;
  void SetThirdPartyCookiesInfo(CookieControlsState controls_state,
                                CookieControlsEnforcement enforcement,
                                CookieBlocking3pcdStatus blocking_status,
                                base::Time expiration) override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_COOKIES_CONTENT_VIEW_H_
