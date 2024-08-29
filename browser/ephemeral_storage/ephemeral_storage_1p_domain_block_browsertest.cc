/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/content/test/engine_test_observer.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

using content::RenderFrameHost;
using content::WebContents;

class EphemeralStorage1pDomainBlockBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorage1pDomainBlockBrowserTest() = default;
  ~EphemeralStorage1pDomainBlockBrowserTest() override = default;

  void SetUpOnMainThread() override {
    EphemeralStorageBrowserTest::SetUpOnMainThread();
    a_site_simple_url_ = https_server_.GetURL("a.com", "/simple.html");
    b_site_simple_url_ = https_server_.GetURL("b.com", "/simple.html");
  }

  void UpdateAdBlockInstanceWithRules(const std::string& rules) {
    source_provider_ =
        std::make_unique<brave_shields::TestFiltersProvider>(rules);

    brave_shields::AdBlockService* ad_block_service =
        g_brave_browser_process->ad_block_service();
    ad_block_service->UseSourceProviderForTest(source_provider_.get());

    auto* engine =
        g_brave_browser_process->ad_block_service()->default_engine_.get();
    EngineTestObserver engine_observer(engine);
    engine_observer.Wait();
  }

  void WaitForAdBlockServiceThreads() {
    scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
        g_brave_browser_process->local_data_files_service()->GetTaskRunner()));
    ASSERT_TRUE(tr_helper->Run());
  }

  void BlockDomainByURL(const GURL& url) {
    UpdateAdBlockInstanceWithRules(base::StrCat({"||", url.host(), "^"}));
  }

  bool IsShowingInterstitial(WebContents* web_contents) {
    return chrome_browser_interstitials::IsShowingInterstitial(web_contents);
  }

  void Click(WebContents* web_contents, const std::string& id) {
    content::RenderFrameHost* frame = web_contents->GetPrimaryMainFrame();
    frame->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("document.getElementById('" + id + "').click();\n"),
        base::NullCallback(), content::ISOLATED_WORLD_ID_GLOBAL);
  }

  void ClickAndWaitForNavigation(WebContents* web_contents,
                                 const std::string& id) {
    content::TestNavigationObserver observer(
        web_contents, 1, content::MessageLoopRunner::QuitMode::DEFERRED);
    Click(web_contents, id);
    observer.Wait();
  }

  WebContents* BlockAndNavigateToBlockedDomain(const GURL& url,
                                               bool is_aggressive,
                                               bool set_dont_warn_again) {
    BlockDomainByURL(url);
    if (is_aggressive) {
      brave_shields::SetCosmeticFilteringControlType(
          content_settings(), brave_shields::ControlType::BLOCK, url);
    }

    chrome::AddTabAt(browser(), GURL("about:blank"), -1, true);
    WebContents* first_party_tab =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::NavigateToURLBlockUntilNavigationsComplete(first_party_tab, url, 1,
                                                        true);

    if (is_aggressive) {
      EXPECT_TRUE(IsShowingInterstitial(first_party_tab));
      if (set_dont_warn_again) {
        Click(first_party_tab, "dont-warn-again-checkbox");
      }
      ClickAndWaitForNavigation(first_party_tab, "primary-button");
    }

    RenderFrameHost* main_frame = first_party_tab->GetPrimaryMainFrame();
    SetValuesInFrame(main_frame, "a.com", "from=a.com");

    ValuesFromFrame main_frame_values = GetValuesFromFrame(main_frame);
    EXPECT_EQ("a.com", main_frame_values.local_storage);
    EXPECT_EQ("a.com", main_frame_values.session_storage);
    EXPECT_EQ("from=a.com", main_frame_values.cookies);

    return first_party_tab;
  }

  void NavigateToBlockedDomainAndExpectEphemeralEnabled() {
    WebContents* first_party_tab =
        BlockAndNavigateToBlockedDomain(a_site_simple_url_, false, false);

    EXPECT_TRUE(GetAllCookies().empty());
    EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
              ContentSetting::CONTENT_SETTING_SESSION_ONLY);

    // After keepalive values should be cleared.
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_simple_url_));
    WaitForCleanupAfterKeepAlive();
    content::NavigateToURLBlockUntilNavigationsComplete(
        first_party_tab, a_site_simple_url_, 1, true);

    ExpectValuesFromFrameAreEmpty(
        FROM_HERE, GetValuesFromFrame(first_party_tab->GetPrimaryMainFrame()));
    EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
              ContentSetting::CONTENT_SETTING_SESSION_ONLY);
  }

  void NavigateToBlockedDomainAndExpectNotEphemeral() {
    WebContents* first_party_tab =
        BlockAndNavigateToBlockedDomain(a_site_simple_url_, false, false);
    EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
              ContentSetting::CONTENT_SETTING_ALLOW);

    // After keepalive main frame values should not be cleared.
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_simple_url_));
    WaitForCleanupAfterKeepAlive();
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), a_site_simple_url_));

    {
      ValuesFromFrame first_party_values =
          GetValuesFromFrame(first_party_tab->GetPrimaryMainFrame());
      EXPECT_EQ("a.com", first_party_values.local_storage);
      EXPECT_EQ("a.com", first_party_values.session_storage);
      EXPECT_EQ("from=a.com", first_party_values.cookies);
    }
    EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
              ContentSetting::CONTENT_SETTING_ALLOW);
  }

  ContentSetting GetCookieSetting(const GURL& url) {
    return content_settings()->GetContentSetting(url, url,
                                                 ContentSettingsType::COOKIES);
  }

 protected:
  std::unique_ptr<brave_shields::TestFiltersProvider> source_provider_;
  GURL a_site_simple_url_;
  GURL b_site_simple_url_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pDomainBlockBrowserTest,
                       FirstPartyEphemeralIsAutoEnabledInNormalBlockingMode) {
  NavigateToBlockedDomainAndExpectEphemeralEnabled();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_simple_url_));
  EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
            ContentSetting::CONTENT_SETTING_ALLOW);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pDomainBlockBrowserTest,
                       FirstPartyEphemeralIsNotEnabledIfCookiesStored) {
  ASSERT_TRUE(content::SetCookie(browser()->profile(), a_site_simple_url_,
                                 "from=a.com;SameSite=None;Secure"));

  NavigateToBlockedDomainAndExpectNotEphemeral();
  EXPECT_EQ(1u, GetAllCookies().size());
  EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
            ContentSetting::CONTENT_SETTING_ALLOW);
}

IN_PROC_BROWSER_TEST_F(
    EphemeralStorage1pDomainBlockBrowserTest,
    FirstPartyEphemeralIsNotEnabledIfLocalStorageDataStored) {
  // Store local storage value in a.com.
  WebContents* first_party_tab = LoadURLInNewTab(a_site_simple_url_);
  SetStorageValueInFrame(first_party_tab->GetPrimaryMainFrame(), "a.com",
                         StorageType::Local);
  // Navigate away to b.com.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_simple_url_));
  // Ensure nothing is cleaned up even after keep alive.
  WaitForCleanupAfterKeepAlive();

  NavigateToBlockedDomainAndExpectNotEphemeral();
  EXPECT_EQ(1u, GetAllCookies().size());
  EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
            ContentSetting::CONTENT_SETTING_ALLOW);
}

IN_PROC_BROWSER_TEST_F(
    EphemeralStorage1pDomainBlockBrowserTest,
    FirstPartyEphemeralIsAutoEnabledInAggressiveBlockingMode) {
  BlockAndNavigateToBlockedDomain(a_site_simple_url_, true, false);
  EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
            ContentSetting::CONTENT_SETTING_SESSION_ONLY);
  EXPECT_EQ(0u, GetAllCookies().size());

  // After keepalive values should be cleared.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_simple_url_));
  WaitForCleanupAfterKeepAlive();
  WebContents* first_party_tab = WebContents::FromRenderFrameHost(
      ui_test_utils::NavigateToURL(browser(), a_site_simple_url_));

  EXPECT_TRUE(IsShowingInterstitial(first_party_tab));
  ClickAndWaitForNavigation(first_party_tab, "primary-button");

  ExpectValuesFromFrameAreEmpty(
      FROM_HERE, GetValuesFromFrame(first_party_tab->GetPrimaryMainFrame()));
  EXPECT_EQ(0u, GetAllCookies().size());
  EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
            ContentSetting::CONTENT_SETTING_SESSION_ONLY);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_simple_url_));
  EXPECT_EQ(GetCookieSetting(a_site_simple_url_),
            ContentSetting::CONTENT_SETTING_ALLOW);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pDomainBlockBrowserTest,
                       FirstPartyEphemeralIsNotEnabledWhenDontWarnChecked) {
  BlockAndNavigateToBlockedDomain(a_site_simple_url_, true, true);

  // After keepalive values should be cleared.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), b_site_simple_url_));
  WaitForCleanupAfterKeepAlive();
  WebContents* first_party_tab = WebContents::FromRenderFrameHost(
      ui_test_utils::NavigateToURL(browser(), a_site_simple_url_));
  EXPECT_FALSE(IsShowingInterstitial(first_party_tab));

  ValuesFromFrame first_party_values =
      GetValuesFromFrame(first_party_tab->GetPrimaryMainFrame());
  EXPECT_EQ("a.com", first_party_values.local_storage);
  EXPECT_EQ("a.com", first_party_values.session_storage);
  EXPECT_EQ("from=a.com", first_party_values.cookies);
  EXPECT_EQ(1u, GetAllCookies().size());
}
