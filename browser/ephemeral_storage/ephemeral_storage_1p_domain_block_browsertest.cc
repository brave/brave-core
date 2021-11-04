/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"

#include "base/test/bind.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

using content::RenderFrameHost;
using content::WebContents;

class EphemeralStorage1pDomainBlockBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorage1pDomainBlockBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBraveFirstPartyEphemeralStorage);
  }
  ~EphemeralStorage1pDomainBlockBrowserTest() override {}

  void SetUpOnMainThread() override {
    EphemeralStorageBrowserTest::SetUpOnMainThread();
  }

  void UpdateAdBlockInstanceWithRules(const std::string& rules,
                                      const std::string& resources = "") {
    brave_shields::AdBlockService* ad_block_service =
        g_brave_browser_process->ad_block_service();
    ad_block_service->GetTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&brave_shields::AdBlockService::ResetForTest,
                                  base::Unretained(ad_block_service), rules,
                                  resources, true));
    WaitForAdBlockServiceThreads();
  }

  void WaitForAdBlockServiceThreads() {
    scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
        g_brave_browser_process->local_data_files_service()->GetTaskRunner()));
    ASSERT_TRUE(tr_helper->Run());
  }

  void BlockDomainByURL(const GURL& url) {
    UpdateAdBlockInstanceWithRules("||" + url.host() + "^");
  }

  bool IsShowingInterstitial(WebContents* web_contents) {
    return chrome_browser_interstitials::IsShowingInterstitial(web_contents);
  }

  void Click(WebContents* web_contents, const std::string& id) {
    content::RenderFrameHost* frame = web_contents->GetMainFrame();
    frame->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("document.getElementById('" + id + "').click();\n"),
        base::NullCallback());
  }

  void ClickAndWaitForNavigation(WebContents* web_contents,
                                 const std::string& id) {
    content::TestNavigationObserver observer(
        web_contents, 1, content::MessageLoopRunner::QuitMode::DEFERRED);
    Click(web_contents, id);
    observer.Wait();
  }

  WebContents* NavigateToBlockedDomain() {
    brave_shields::SetCosmeticFilteringControlType(
        content_settings(), brave_shields::ControlType::BLOCK,
        a_site_ephemeral_storage_url_);
    BlockDomainByURL(a_site_ephemeral_storage_url_);

    WebContents* first_party_tab =
        LoadURLInNewTab(a_site_ephemeral_storage_url_);
    EXPECT_TRUE(IsShowingInterstitial(first_party_tab));
    Click(first_party_tab, "dont-warn-again-checkbox");
    ClickAndWaitForNavigation(first_party_tab, "primary-button");

    // We set a value in the page where all the frames are first-party.
    SetValuesInFrames(first_party_tab, "a.com", "from=a.com");

    {
      ValuesFromFrames first_party_values =
          GetValuesFromFrames(first_party_tab);
      EXPECT_EQ("a.com", first_party_values.main_frame.local_storage);
      EXPECT_EQ("a.com", first_party_values.iframe_1.local_storage);
      EXPECT_EQ("a.com", first_party_values.iframe_2.local_storage);

      EXPECT_EQ("a.com", first_party_values.main_frame.session_storage);
      EXPECT_EQ("a.com", first_party_values.iframe_1.session_storage);
      EXPECT_EQ("a.com", first_party_values.iframe_2.session_storage);

      EXPECT_EQ("from=a.com", first_party_values.main_frame.cookies);
      EXPECT_EQ("from=a.com", first_party_values.iframe_1.cookies);
      EXPECT_EQ("from=a.com", first_party_values.iframe_2.cookies);
    }

    return first_party_tab;
  }

  void NavigateToBlockedDomainAndExpectEphemeralEnabled() {
    WebContents* first_party_tab = NavigateToBlockedDomain();

    // After keepalive values should be cleared.
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
    WaitForCleanupAfterKeepAlive();
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

    ExpectValuesFromFramesAreEmpty(FROM_HERE,
                                   GetValuesFromFrames(first_party_tab));
  }

  void NavigateToBlockedDomainAndExpectNotEphemeral() {
    WebContents* first_party_tab = NavigateToBlockedDomain();

    // After keepalive main frame values should not be cleared.
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
    WaitForCleanupAfterKeepAlive();
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

    {
      ValuesFromFrames first_party_values =
          GetValuesFromFrames(first_party_tab);
      EXPECT_EQ("a.com", first_party_values.main_frame.local_storage);
      EXPECT_EQ(nullptr, first_party_values.iframe_1.local_storage);
      EXPECT_EQ(nullptr, first_party_values.iframe_2.local_storage);

      EXPECT_EQ("a.com", first_party_values.main_frame.session_storage);
      EXPECT_EQ(nullptr, first_party_values.iframe_1.session_storage);
      EXPECT_EQ(nullptr, first_party_values.iframe_2.session_storage);

      EXPECT_EQ("from=a.com", first_party_values.main_frame.cookies);
      EXPECT_EQ("", first_party_values.iframe_1.cookies);
      EXPECT_EQ("", first_party_values.iframe_2.cookies);
    }
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(
    EphemeralStorage1pDomainBlockBrowserTest,
    FirstPartyEphemeralIsEnabledAfterIntersisitalProcessing) {
  brave_shields::SetCosmeticFilteringControlType(
      content_settings(), brave_shields::ControlType::BLOCK,
      a_site_ephemeral_storage_url_);
  BlockDomainByURL(a_site_ephemeral_storage_url_);

  NavigateToBlockedDomainAndExpectEphemeralEnabled();
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pDomainBlockBrowserTest,
                       FirstPartyEphemeralIsNotEnabledIfCookiesStored) {
  ASSERT_TRUE(content::SetCookie(browser()->profile(),
                                 a_site_ephemeral_storage_url_,
                                 "from=a.com;SameSite=None;Secure"));

  NavigateToBlockedDomainAndExpectNotEphemeral();
}

IN_PROC_BROWSER_TEST_F(
    EphemeralStorage1pDomainBlockBrowserTest,
    FirstPartyEphemeralIsNotEnabledIfLocalStorageDataStored) {
  // Store local storage value in a.com.
  WebContents* first_party_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  SetStorageValueInFrame(first_party_tab->GetMainFrame(), "a.com",
                         StorageType::Local);
  // Navigate away to b.com.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));

  NavigateToBlockedDomainAndExpectNotEphemeral();
}
