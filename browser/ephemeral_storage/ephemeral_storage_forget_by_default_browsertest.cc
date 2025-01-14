/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "net/base/features.h"

using content::RenderFrameHost;
using content::WebContents;

namespace ephemeral_storage {

class EphemeralStorageForgetByDefaultBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorageForgetByDefaultBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBraveForgetFirstPartyStorage);
  }

  ~EphemeralStorageForgetByDefaultBrowserTest() override = default;

  void SetAndCheckValuesInFrames(WebContents* web_contents,
                                 std::string storage_value,
                                 std::string cookie_value) {
    SetValuesInFrames(web_contents, storage_value, cookie_value);

    ValuesFromFrames first_party_values = GetValuesFromFrames(web_contents);
    EXPECT_EQ(storage_value, first_party_values.main_frame.local_storage);
    EXPECT_EQ(storage_value, first_party_values.iframe_1.local_storage);
    EXPECT_EQ(storage_value, first_party_values.iframe_2.local_storage);

    EXPECT_EQ(storage_value, first_party_values.main_frame.session_storage);
    EXPECT_EQ(storage_value, first_party_values.iframe_1.session_storage);
    EXPECT_EQ(storage_value, first_party_values.iframe_2.session_storage);

    EXPECT_EQ(cookie_value, first_party_values.main_frame.cookies);
    EXPECT_EQ(cookie_value, first_party_values.iframe_1.cookies);
    EXPECT_EQ(cookie_value, first_party_values.iframe_2.cookies);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       ForgetFirstParty) {
  EXPECT_EQ(
      brave_shields::GetCookieControlType(
          content_settings(),
          CookieSettingsFactory::GetForProfile(browser()->profile()).get(),
          a_site_ephemeral_storage_url_),
      brave_shields::ControlType::BLOCK_THIRD_PARTY);

  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, a_site_ephemeral_storage_url_);

  WebContents* first_party_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);

  // We set a value in the page where all the frames are first-party.
  SetAndCheckValuesInFrames(first_party_tab, "a.com", "from=a.com");

  // After keepalive values should be cleared.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  ExpectValuesFromFramesAreEmpty(FROM_HERE,
                                 GetValuesFromFrames(first_party_tab));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       ForgetFirstPartyOnSubdomain) {
  EXPECT_EQ(
      brave_shields::GetCookieControlType(
          content_settings(),
          CookieSettingsFactory::GetForProfile(browser()->profile()).get(),
          a_site_ephemeral_storage_url_),
      brave_shields::ControlType::BLOCK_THIRD_PARTY);

  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, a_site_ephemeral_storage_url_);

  const GURL sub_a_site_ephemeral_storage_url =
      https_server_.GetURL("sub.a.com", "/ephemeral_storage.html");
  WebContents* first_party_tab =
      LoadURLInNewTab(sub_a_site_ephemeral_storage_url);

  // We set a value in the page where all the frames are first-party.
  SetAndCheckValuesInFrames(first_party_tab, "a.com", "from=a.com");

  // After keepalive values should be cleared.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           sub_a_site_ephemeral_storage_url));

  ExpectValuesFromFramesAreEmpty(FROM_HERE,
                                 GetValuesFromFrames(first_party_tab));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       ForgetFirstPartyOnRootDomainIfEnabledOnSubdomain) {
  const GURL sub_a_site_ephemeral_storage_url =
      https_server_.GetURL("sub.a.com", "/ephemeral_storage.html");
  EXPECT_EQ(
      brave_shields::GetCookieControlType(
          content_settings(),
          CookieSettingsFactory::GetForProfile(browser()->profile()).get(),
          sub_a_site_ephemeral_storage_url),
      brave_shields::ControlType::BLOCK_THIRD_PARTY);

  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, sub_a_site_ephemeral_storage_url);

  WebContents* first_party_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);

  // We set a value in the page where all the frames are first-party.
  SetAndCheckValuesInFrames(first_party_tab, "a.com", "from=a.com");

  // After keepalive values should be cleared.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  ExpectValuesFromFramesAreEmpty(FROM_HERE,
                                 GetValuesFromFrames(first_party_tab));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       DontForgetIfSubdomainIsOpened) {
  const GURL sub_a_site_ephemeral_storage_url =
      https_server_.GetURL("sub.a.com", "/ephemeral_storage.html");
  EXPECT_EQ(
      brave_shields::GetCookieControlType(
          content_settings(),
          CookieSettingsFactory::GetForProfile(browser()->profile()).get(),
          sub_a_site_ephemeral_storage_url),
      brave_shields::ControlType::BLOCK_THIRD_PARTY);

  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, sub_a_site_ephemeral_storage_url);

  WebContents* sub_first_party_tab =
      LoadURLInNewTab(sub_a_site_ephemeral_storage_url);

  // We set a value in the page where all the frames are first-party.
  EXPECT_EQ(0u, GetAllCookies().size());
  SetAndCheckValuesInFrames(sub_first_party_tab, "sub.a.com", "from=sub.a.com");
  EXPECT_EQ(1u, GetAllCookies().size());

  ASSERT_TRUE(LoadURLInNewTab(a_site_ephemeral_storage_url_));

  // Navigate sub.a.com. After keepalive values should not be cleared.
  ASSERT_TRUE(content::NavigateToURL(sub_first_party_tab,
                                     b_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();

  EXPECT_EQ(1u, GetAllCookies().size());
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       ForgetFirstPartyInheritedInIncognito) {
  Browser* incognito_browser = CreateIncognitoBrowser(nullptr);
  EXPECT_EQ(
      brave_shields::GetCookieControlType(
          HostContentSettingsMapFactory::GetForProfile(
              incognito_browser->profile()),
          CookieSettingsFactory::GetForProfile(incognito_browser->profile())
              .get(),
          a_site_ephemeral_storage_url_),
      brave_shields::ControlType::BLOCK_THIRD_PARTY);

  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, a_site_ephemeral_storage_url_);

  EXPECT_TRUE(brave_shields::GetForgetFirstPartyStorageEnabled(
      HostContentSettingsMapFactory::GetForProfile(
          incognito_browser->profile()),
      a_site_ephemeral_storage_url_));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(incognito_browser,
                                           a_site_ephemeral_storage_url_));
  auto* incognito_web_contents =
      incognito_browser->tab_strip_model()->GetActiveWebContents();

  // We set a value in the page where all the frames are first-party.
  SetAndCheckValuesInFrames(incognito_web_contents, "a.com", "from=a.com");

  // After keepalive values should be cleared.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(incognito_browser,
                                           b_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive(incognito_browser->profile());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(incognito_browser,
                                           a_site_ephemeral_storage_url_));

  ExpectValuesFromFramesAreEmpty(FROM_HERE,
                                 GetValuesFromFrames(incognito_web_contents));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       NavigationCookiesAreCleared) {
  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, a_site_ephemeral_storage_url_);
  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, b_site_ephemeral_storage_url_);

  const GURL a_site_set_cookie_url = https_server_.GetURL(
      "a.com", "/set-cookie?name=acom;path=/;SameSite=None;Secure;Max-Age=600");
  const GURL b_site_set_cookie_url = https_server_.GetURL(
      "b.com", "/set-cookie?name=bcom;path=/;SameSite=None;Secure;Max-Age=600");

  WebContents* site_a_set_cookies = LoadURLInNewTab(a_site_set_cookie_url);
  WebContents* site_b_set_cookies = LoadURLInNewTab(b_site_set_cookie_url);
  WebContents* site_a = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_b = LoadURLInNewTab(b_site_ephemeral_storage_url_);

  // Default cookie storage request should return non empty results.
  EXPECT_FALSE(content::GetCookies(browser()->profile(),
                                   https_server_.GetURL("a.com", "/"))
                   .empty());
  EXPECT_FALSE(content::GetCookies(browser()->profile(),
                                   https_server_.GetURL("b.com", "/"))
                   .empty());

  // JS cookie request should return valid results.
  EXPECT_EQ("name=acom",
            GetCookiesInFrame(site_a_set_cookies->GetPrimaryMainFrame()));
  EXPECT_EQ("name=bcom",
            GetCookiesInFrame(site_b_set_cookies->GetPrimaryMainFrame()));
  EXPECT_EQ("name=acom", GetCookiesInFrame(site_a->GetPrimaryMainFrame()));

  // Navigating to a new TLD should clear all ephemeral cookies after keep-alive
  // timeout.
  ASSERT_TRUE(content::NavigateToURL(site_a_set_cookies,
                                     c_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_b_set_cookies,
                                     c_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_a, c_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_b, c_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();
  ASSERT_TRUE(content::NavigateToURL(site_a, a_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_b, b_site_ephemeral_storage_url_));

  ValuesFromFrames values_site_a = GetValuesFromFrames(site_a);
  EXPECT_EQ("", values_site_a.main_frame.cookies);
  EXPECT_EQ("", values_site_a.iframe_1.cookies);
  EXPECT_EQ("", values_site_a.iframe_2.cookies);

  ValuesFromFrames values_site_b = GetValuesFromFrames(site_b);
  EXPECT_EQ("", values_site_b.main_frame.cookies);
  EXPECT_EQ("", values_site_b.iframe_1.cookies);
  EXPECT_EQ("", values_site_b.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       PRE_ForgetFirstPartyAfterRestart) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  brave_shields::SetForgetFirstPartyStorageEnabled(content_settings(), true,
                                                   a_site_set_cookie_url);

  // Cookies should NOT exist for a.com.
  EXPECT_EQ(0u, GetAllCookies().size());

  EXPECT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));

  // Cookies SHOULD exist for a.com.
  EXPECT_EQ(1u, GetAllCookies().size());

  // Navigate to b.com to activate a deferred cleanup for a.com.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       ForgetFirstPartyAfterRestart) {
  EXPECT_EQ(1u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(0u, GetAllCookies().size());
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       PRE_DontForgetFirstPartyIfSubDomainIsOpened) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  brave_shields::SetForgetFirstPartyStorageEnabled(content_settings(), true,
                                                   a_site_set_cookie_url);

  // Cookies should NOT exist for a.com.
  EXPECT_EQ(0u, GetAllCookies().size());

  EXPECT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));

  // Cookies SHOULD exist for a.com.
  EXPECT_EQ(1u, GetAllCookies().size());

  // Navigate to b.com to activate a deferred cleanup for a.com.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));

  // Open sub.a.com in another tab to stop the deferred cleanup for a.com.
  const GURL sub_a_site_ephemeral_storage_url =
      https_server_.GetURL("sub.a.com", "/ephemeral_storage.html");
  EXPECT_TRUE(LoadURLInNewTab(sub_a_site_ephemeral_storage_url));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       DontForgetFirstPartyIfSubDomainIsOpened) {
  EXPECT_EQ(0u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(1u, GetAllCookies().size());
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       DisabledShieldsDontForgetFirstParty) {
  brave_shields::SetForgetFirstPartyStorageEnabled(
      content_settings(), true, a_site_ephemeral_storage_url_);
  brave_shields::SetBraveShieldsEnabled(content_settings(), false,
                                        a_site_ephemeral_storage_url_);

  // Navigate to a.com which includes b.com.
  WebContents* site_a_tab_network_cookies =
      LoadURLInNewTab(a_site_ephemeral_storage_with_network_cookies_url_);
  http_request_monitor_.Clear();

  // Cookies should be stored in persistent storage for the main frame and a
  // third party frame.
  EXPECT_EQ(2u, GetAllCookies().size());

  // Navigate to other website and ensure no a.com/b.com cookies are sent (they
  // are third-party and ephemeral inside c.com).
  ASSERT_TRUE(content::NavigateToURL(site_a_tab_network_cookies,
                                     c_site_ephemeral_storage_url_));
  EXPECT_FALSE(http_request_monitor_.HasHttpRequestWithCookie(
      a_site_ephemeral_storage_url_, "name=acom_simple"));
  EXPECT_FALSE(http_request_monitor_.HasHttpRequestWithCookie(
      b_site_ephemeral_storage_url_, "name=bcom_simple"));
  WaitForCleanupAfterKeepAlive();
  http_request_monitor_.Clear();

  // a.com and b.com cookies should be intact.
  EXPECT_EQ(2u, GetAllCookies().size());

  // Navigate to a.com again and expect a.com and b.com cookies are sent with
  // headers.
  WebContents* site_a_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      a_site_ephemeral_storage_url_, "name=acom_simple"));
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      b_site_ephemeral_storage_url_, "name=bcom_simple"));

  // Make sure cookies are also accessible via JS.
  ValuesFromFrames site_a_tab_values = GetValuesFromFrames(site_a_tab);
  EXPECT_EQ("name=acom_simple", site_a_tab_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab_values.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultBrowserTest,
                       DisabledShieldsDontForgetFirstPartyOnSubdomain) {
  const GURL sub_a_site_set_cookie_url(
      "https://sub.a.com/set-cookie?name=sub_acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  const GURL sub_a_site_ephemeral_storage_with_network_cookies_url =
      https_server_.GetURL("sub.a.com",
                           "/ephemeral_storage_with_network_cookies.html");
  const GURL sub_a_site_ephemeral_storage_url =
      https_server_.GetURL("sub.a.com", "/ephemeral_storage.html");
  const GURL a_com_simple_url = https_server_.GetURL("a.com", "/simple.html");

  brave_shields::SetForgetFirstPartyStorageEnabled(content_settings(), true,
                                                   sub_a_site_set_cookie_url);
  brave_shields::SetBraveShieldsEnabled(content_settings(), false,
                                        sub_a_site_set_cookie_url);

  WebContents* sub_site_a_tab_network_cookies =
      LoadURLInNewTab(sub_a_site_set_cookie_url);

  // Navigate to a.com which includes b.com.
  ASSERT_TRUE(content::NavigateToURL(
      sub_site_a_tab_network_cookies,
      sub_a_site_ephemeral_storage_with_network_cookies_url));
  http_request_monitor_.Clear();

  // Cookies should be stored in persistent storage for the main frame, 1p frame
  // and a third party frame.
  EXPECT_EQ(3u, GetAllCookies().size());

  // Navigate to other website and ensure no a.com/b.com cookies are sent (they
  // are third-party and ephemeral inside c.com).
  ASSERT_TRUE(content::NavigateToURL(sub_site_a_tab_network_cookies,
                                     c_site_ephemeral_storage_url_));
  EXPECT_FALSE(http_request_monitor_.HasHttpRequestWithCookie(
      sub_a_site_ephemeral_storage_url, "name=sub_acom"));
  EXPECT_FALSE(http_request_monitor_.HasHttpRequestWithCookie(
      a_com_simple_url, "name=acom_simple"));
  EXPECT_FALSE(http_request_monitor_.HasHttpRequestWithCookie(
      b_site_ephemeral_storage_url_, "name=bcom_simple"));
  WaitForCleanupAfterKeepAlive();
  http_request_monitor_.Clear();

  // sub.a.com, a.com and b.com cookies should be intact.
  EXPECT_EQ(3u, GetAllCookies().size());

  // Navigate to sub.a.com again and expect sub.a.com, a.com and b.com cookies
  // are sent with headers.
  WebContents* sub_site_a_tab =
      LoadURLInNewTab(sub_a_site_ephemeral_storage_url);
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      sub_a_site_ephemeral_storage_url, "name=sub_acom"));
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      a_com_simple_url, "name=acom_simple"));
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      b_site_ephemeral_storage_url_, "name=bcom_simple"));

  // Make sure cookies are also accessible via JS.
  ValuesFromFrames site_a_tab_values = GetValuesFromFrames(sub_site_a_tab);
  EXPECT_EQ("name=sub_acom", site_a_tab_values.main_frame.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab_values.iframe_1.cookies);
  EXPECT_EQ("name=bcom_simple", site_a_tab_values.iframe_2.cookies);
}

class EphemeralStorageForgetByDefaultIsDefaultBrowserTest
    : public EphemeralStorageForgetByDefaultBrowserTest {
 public:
  EphemeralStorageForgetByDefaultIsDefaultBrowserTest() {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        net::features::kBraveForgetFirstPartyStorage,
        {{"BraveForgetFirstPartyStorageByDefault", "true"}});
  }
  ~EphemeralStorageForgetByDefaultIsDefaultBrowserTest() override = default;

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultIsDefaultBrowserTest,
                       PRE_ForgetFirstPartyAfterRestart) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  EXPECT_TRUE(brave_shields::GetForgetFirstPartyStorageEnabled(
      content_settings(), a_site_set_cookie_url));

  // Cookies should NOT exist for a.com.
  EXPECT_EQ(0u, GetAllCookies().size());

  EXPECT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));

  // Cookies SHOULD exist for a.com.
  EXPECT_EQ(1u, GetAllCookies().size());

  // Navigate to b.com to activate a deferred cleanup for a.com.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultIsDefaultBrowserTest,
                       ForgetFirstPartyAfterRestart) {
  EXPECT_EQ(1u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(0u, GetAllCookies().size());
}

class EphemeralStorageForgetByDefaultDisabledBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorageForgetByDefaultDisabledBrowserTest() {
    if (IsPreTest()) {
      scoped_feature_list_.InitAndEnableFeature(
          net::features::kBraveForgetFirstPartyStorage);
    } else {
      scoped_feature_list_.InitWithFeatures(
          {}, {net::features::kBraveForgetFirstPartyStorage,
               net::features::kThirdPartyStoragePartitioning});
    }
  }
  ~EphemeralStorageForgetByDefaultDisabledBrowserTest() override = default;

  static bool IsPreTest() {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    return std::string_view(test_info->name()).starts_with("PRE_");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultDisabledBrowserTest,
                       PRE_ForgetFirstPartyIsNotActive) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  brave_shields::SetForgetFirstPartyStorageEnabled(content_settings(), true,
                                                   a_site_set_cookie_url);
  EXPECT_TRUE(brave_shields::GetForgetFirstPartyStorageEnabled(
      content_settings(), a_site_set_cookie_url));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultDisabledBrowserTest,
                       ForgetFirstPartyIsNotActive) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  const GURL a_com_empty("https://a.com/empty.html");
  EXPECT_EQ(
      brave_shields::GetCookieControlType(
          content_settings(),
          CookieSettingsFactory::GetForProfile(browser()->profile()).get(),
          a_site_set_cookie_url),
      brave_shields::ControlType::BLOCK_THIRD_PARTY);

  EXPECT_EQ(0u, GetAllCookies().size());
  EXPECT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));
  EXPECT_EQ(1u, GetAllCookies().size());

  // After keepalive a.com values should not be cleared.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), a_com_empty));

  EXPECT_EQ(1u, GetAllCookies().size());
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultDisabledBrowserTest,
                       PRE_ForgetFirstPartyDoesntClearIfWasActive) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  brave_shields::SetForgetFirstPartyStorageEnabled(content_settings(), true,
                                                   a_site_set_cookie_url);
  EXPECT_TRUE(brave_shields::GetForgetFirstPartyStorageEnabled(
      content_settings(), a_site_set_cookie_url));

  ASSERT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));
  EXPECT_EQ(1u, GetAllCookies().size());

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultDisabledBrowserTest,
                       ForgetFirstPartyDoesntClearIfWasActive) {
  const GURL a_com_empty("https://a.com/empty.html");
  EXPECT_EQ(
      brave_shields::GetCookieControlType(
          content_settings(),
          CookieSettingsFactory::GetForProfile(browser()->profile()).get(),
          a_com_empty),
      brave_shields::ControlType::BLOCK_THIRD_PARTY);

  EXPECT_EQ(1u, GetAllCookies().size());
}

class EphemeralStorageForgetByDefaultIncognitoBrowserTest
    : public EphemeralStorageForgetByDefaultBrowserTest {
 public:
  EphemeralStorageForgetByDefaultIncognitoBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBraveForgetFirstPartyStorage);
  }
  ~EphemeralStorageForgetByDefaultIncognitoBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    EphemeralStorageForgetByDefaultBrowserTest::SetUpCommandLine(command_line);
    if (IsPreTestToEnableIncognito()) {
      command_line->AppendSwitch(switches::kIncognito);
    }
  }

  static bool IsPreTestToEnableIncognito() {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    return base::StartsWith(test_info->name(), "PRE_DontForgetFirstParty");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultIncognitoBrowserTest,
                       PRE_PRE_DontForgetFirstPartyIfNoBrowserWindowIsActive) {
  // This PRE test runs in a normal profile and sets a single cookie.

  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  brave_shields::SetForgetFirstPartyStorageEnabled(content_settings(), true,
                                                   a_site_set_cookie_url);

  // Cookies should NOT exist for a.com.
  EXPECT_EQ(0u, GetAllCookies().size());

  EXPECT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));

  // Cookies SHOULD exist for a.com.
  EXPECT_EQ(1u, GetAllCookies().size());

  // Navigate to b.com to activate a deferred cleanup for a.com.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultIncognitoBrowserTest,
                       PRE_DontForgetFirstPartyIfNoBrowserWindowIsActive) {
  // This PRE test runs in incognito mode, meaning no normal browser window is
  // active. This should prevent the cleanup in the normal profile.

  // Ensure no normal browser window is active.
  EXPECT_TRUE(browser()->profile()->IsOffTheRecord());
  for (const auto& browser_instance : *BrowserList::GetInstance()) {
    EXPECT_TRUE(browser_instance->profile()->IsOffTheRecord());
    EXPECT_EQ(browser_instance->profile(), browser()->profile());
  }

  EXPECT_EQ(0u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(0u, WaitForCleanupAfterKeepAlive(
                    browser()->profile()->GetOriginalProfile()));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageForgetByDefaultIncognitoBrowserTest,
                       DontForgetFirstPartyIfNoBrowserWindowIsActive) {
  // Expect the cleanup did not happen (yet).
  EXPECT_EQ(1u, GetAllCookies().size());

  // But it is queued and should happen eventually.
  EXPECT_EQ(1u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(0u, GetAllCookies().size());
}

}  // namespace ephemeral_storage
