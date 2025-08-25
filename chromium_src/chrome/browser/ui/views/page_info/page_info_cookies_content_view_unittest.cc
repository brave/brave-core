/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/ui/views/page_info/page_info_cookies_content_view_unittest.cc>

using PageInfoCookiesContentViewBaseTestClassBraveOverrides =
    PageInfoCookiesContentViewBaseTestClass;

TEST_F(PageInfoCookiesContentViewBaseTestClassBraveOverrides,
       ThirdPartyCoookiesInfoIsRemoved) {
  // PageInfoCookiesContentViewBaseTestClass::SetUp already creates the view,
  // so the container and the label should be removed by now.
  EXPECT_EQ(nullptr, third_party_cookies_container());
  EXPECT_EQ(nullptr, third_party_cookies_description_wrapper());
  EXPECT_EQ(nullptr, third_party_cookies_description_label());
}
