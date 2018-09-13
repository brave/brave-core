/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave_cookie_settings.h"
#include "url/gurl.h"

#include "testing/gtest/include/gtest/gtest.h"

class BraveCookieSettingsTest : public content_settings::BraveCookieSettings {
  ~BraveCookieSettingsTest() override;
};

TEST(BraveCookieSettingsTest, BlockDifferentSubdomainTest) {
  EXPECT_EQ(true, BraveCookieSettingsTest::IsInSubdomain(GURL("http://a.brave.com"),GURL("http://brave.com")));
  EXPECT_EQ(false, BraveCookieSettingsTest::IsInSubdomain(GURL("http://brave.com"),GURL("http://a.brave.com")));
  EXPECT_EQ(false, BraveCookieSettingsTest::IsInSubdomain(GURL("http://a.brave.com"),GURL("http://b.brave.com")));
}
