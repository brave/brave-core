/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/content_settings/core/common/cookie_settings_base_unittest.cc>

namespace content_settings {
namespace {

// Cookie access for a request that `site_for_cookies` makes third-party,
// with third-party cookies blocked and every cookie setting at ALLOW.
class BraveCookieSettingsBaseTest : public CookieSettingsBaseTest {
 protected:
  bool IsFullCookieAccessAllowed(const GURL& url,
                                 const net::SiteForCookies& site_for_cookies,
                                 const GURL& top_frame_url) {
    return settings_.IsFullCookieAccessAllowed(
        url, site_for_cookies, url::Origin::Create(top_frame_url), overrides_,
        /*cookie_partition_key=*/std::nullopt);
  }

  const GURL third_party_url_{"https://3p.com/resource"};
  const net::CookieSettingOverrides overrides_{
      net::CookieSettingOverride::kForceDisableThirdPartyCookies};
  CallbackCookieSettings settings_{CONTENT_SETTING_ALLOW};
};

// A request whose top frame is its own site stays third-party while
// `site_for_cookies` names another site. Browser-process requests that omit
// isolation info get it assigned this way from their own URL.
TEST_F(BraveCookieSettingsBaseTest,
       TopFrameOfRequestSiteDoesNotAllowThirdPartyCookies) {
  EXPECT_FALSE(IsFullCookieAccessAllowed(third_party_url_, site_for_cookies_,
                                         /*top_frame_url=*/third_party_url_));

  // Controls: the same request is first-party once `site_for_cookies` agrees,
  // and third-party when the top frame is the other site.
  EXPECT_TRUE(IsFullCookieAccessAllowed(
      third_party_url_, net::SiteForCookies::FromUrl(third_party_url_),
      /*top_frame_url=*/third_party_url_));
  EXPECT_FALSE(IsFullCookieAccessAllowed(third_party_url_, site_for_cookies_,
                                         /*top_frame_url=*/url_));
}

// A first-party frame nested in a third-party frame keeps its cookies. Its
// `site_for_cookies` is null because its ancestors are not all same-site.
TEST_F(BraveCookieSettingsBaseTest, NestedFirstPartyFrameAllowsCookies) {
  EXPECT_TRUE(IsFullCookieAccessAllowed(url_, net::SiteForCookies(),
                                        /*top_frame_url=*/url_));
}

// wp.com and wordpress.com share cookies when one embeds the other.
TEST_F(BraveCookieSettingsBaseTest, WordpressEntityAllowsCookies) {
  const GURL wordpress("https://wordpress.com/");

  EXPECT_TRUE(IsFullCookieAccessAllowed(GURL("https://s0.wp.com/resource"),
                                        net::SiteForCookies::FromUrl(wordpress),
                                        /*top_frame_url=*/wordpress));
}

}  // namespace
}  // namespace content_settings
