/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/ui/views/page_info/page_info_cookies_content_view_unittest.cc>

using PageInfoCookiesContentViewBaseTestClassBraveOverrides =
    PageInfoCookiesContentViewBaseTestClass;

TEST_F(PageInfoCookiesContentViewBaseTestClassBraveOverrides,
       ThirdPartyCoookiesInfoIsHidden) {
  PageInfoCookiesContentView::CookiesInfo cookie_info =
      DefaultCookieInfoForTests();
  content_view()->SetCookieInfo(cookie_info);

  EXPECT_FALSE(third_party_cookies_container()->GetVisible());
  EXPECT_FALSE(third_party_cookies_description_wrapper()->GetVisible());
}

TEST_F(PageInfoCookiesContentViewBaseTestClassBraveOverrides,
       ThirdPartyCoookiesInfoIsHiddenInIncognitoMode) {
  PageInfoCookiesContentView::CookiesInfo cookie_info =
      DefaultCookieInfoForTests();
  cookie_info.controls_state = CookieControlsState::kAllowed3pc;
  cookie_info.is_incognito = true;
  content_view()->SetCookieInfo(cookie_info);
  EXPECT_FALSE(third_party_cookies_container()->GetVisible());
  EXPECT_FALSE(third_party_cookies_description_wrapper()->GetVisible());
}
