/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/ad_block_service_browsertest.h"

#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "url/gurl.h"

using brave_shields::ControlType;
using brave_shields::ResetBraveShieldsEnabled;
using brave_shields::SetBraveShieldsEnabled;
using brave_shields::SetCosmeticFilteringControlType;
using brave_shields::features::kBraveDomainBlock;

class DomainBlockTestBase : public AdBlockServiceTest {
 public:
  DomainBlockTestBase() {}
  DomainBlockTestBase(const DomainBlockTestBase&) = delete;
  DomainBlockTestBase& operator=(const DomainBlockTestBase&) = delete;

  void SetUp() override {
    request_count_ = 0;
    embedded_test_server()->RegisterRequestMonitor(base::BindLambdaForTesting(
        [&](const net::test_server::HttpRequest& request) {
          request_count_ += 1;
        }));
    content::SetupCrossSiteRedirector(embedded_test_server());
    AdBlockServiceTest::SetUp();
  }

  void BlockDomainByURL(const GURL& url) {
    UpdateAdBlockInstanceWithRules("||" + url.host() + "^");
  }

  content::WebContents* web_contents() {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    DCHECK(web_contents);
    return web_contents;
  }

  bool IsShowingInterstitial() {
    return chrome_browser_interstitials::IsShowingInterstitial(web_contents());
  }

  void NavigateTo(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    content::RenderFrameHost* frame = web_contents()->GetMainFrame();
    ASSERT_TRUE(WaitForRenderFrameReady(frame));
  }

  void Click(const std::string& id) {
    content::RenderFrameHost* frame = web_contents()->GetMainFrame();
    frame->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("document.getElementById('" + id + "').click();\n"),
        base::NullCallback());
  }

  void ClickAndWaitForNavigation(const std::string& id) {
    content::TestNavigationObserver observer(web_contents());
    Click(id);
    observer.WaitForNavigationFinished();
  }

 protected:
  int request_count_;
};

class DomainBlockTest : public DomainBlockTestBase {
 public:
  DomainBlockTest() { feature_list_.InitAndEnableFeature(kBraveDomainBlock); }

 private:
  base::test::ScopedFeatureList feature_list_;
};

class DomainBlockDisabledTest : public DomainBlockTestBase {
 public:
  DomainBlockDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveDomainBlock);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(DomainBlockTest, ShowInterstitial) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url);
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, ShowInterstitialAndProceed) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url);
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Proceed anyway" button. This should navigate to the
  // originally requested page.
  ClickAndWaitForNavigation("primary-button");
  ASSERT_FALSE(IsShowingInterstitial());
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, ShowInterstitialAndReload) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url);
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Attempt to reload page, which should again be interrupted by the
  // interstitial.
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  ASSERT_TRUE(IsShowingInterstitial());
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, ProceedAndReload) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url);
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Proceed anyway" button. This should navigate to the
  // originally requested page.
  ClickAndWaitForNavigation("primary-button");
  ASSERT_FALSE(IsShowingInterstitial());
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());

  // Reload page. This should work normally and not by interrupted by the
  // interstitial, because we chose to proceed in this tab, and that decision
  // should persist for the lifetime of the tab.
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  ASSERT_FALSE(IsShowingInterstitial());
  content::TitleWatcher watcher2(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher2.WaitAndGetTitle());
  EXPECT_EQ(web_contents()->GetURL().host(), "a.com");
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, ProceedDoesNotAffectNewTabs) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url);
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Proceed anyway" button. This should navigate to the
  // originally requested page.
  ClickAndWaitForNavigation("primary-button");
  ASSERT_FALSE(IsShowingInterstitial());
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());

  // Open a new tab and navigate to a page on a.com. This should be interrupted
  // by the domain block interstitial, because the permission we gave by
  // clicking "Proceed anyway" in the other tab is tab-specific.
  ui_test_utils::AllBrowserTabAddedWaiter new_tab;
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  new_tab.Wait();
  ASSERT_TRUE(IsShowingInterstitial());
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, DontWarnAgainAndProceed) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url);
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Don't warn again" checkbox. This should not navigate.
  // We should still be on the interstitial page.
  Click("dont-warn-again-checkbox");
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Proceed anyway" button. This should save the "don't warn
  // again" choice and navigate to the originally requested page.
  ClickAndWaitForNavigation("primary-button");
  WaitForAdBlockServiceThreads();
  ASSERT_FALSE(IsShowingInterstitial());
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());

  // Open a new tab and navigate to a page on a.com. This should navigate
  // directly, because we previously saved the "don't warn again" choice for
  // this domain and are now respecting that choice.
  ui_test_utils::AllBrowserTabAddedWaiter new_tab;
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  new_tab.Wait();
  ASSERT_FALSE(IsShowingInterstitial());
  EXPECT_EQ(web_contents()->GetURL().host(), "a.com");
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, ShowInterstitialAndGoBack) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url_a = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK,
                                  url_a);
  GURL url_b = embedded_test_server()->GetURL("b.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK,
                                  url_b);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url_a);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block b.com, then attempt to navigate to a page on b.com, which should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url_b);
  NavigateTo(url_b);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Go back" button. This should return to previous page on
  // a.com.
  ClickAndWaitForNavigation("back-button");
  ASSERT_FALSE(IsShowingInterstitial());
  EXPECT_EQ(web_contents()->GetURL().host(), "a.com");
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, NoFetch) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  ASSERT_EQ(0, request_count_);
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);
  BlockDomainByURL(url);
  ui_test_utils::AllBrowserTabAddedWaiter new_tab;
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  new_tab.Wait();

  // Should be showing domain blocked interstitial page.
  ASSERT_TRUE(IsShowingInterstitial());

  // Should be zero network traffic (not even a favicon fetch).
  ASSERT_EQ(0, request_count_);
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, NoThirdPartyInterstitial) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  ASSERT_TRUE(g_brave_browser_process->ad_block_custom_filters_service()
                  ->UpdateCustomFilters("||b.com^$third-party"));

  GURL url = embedded_test_server()->GetURL("a.com", "/simple_link.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);
  GURL cross_url =
      embedded_test_server()->GetURL("a.com", "/cross-site/b.com/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK,
                                  cross_url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Navigate to a page on the third-party b.com. There should be no
  // interstitial shown.
  EXPECT_EQ(true,
            EvalJs(web_contents(), "clickLink('" + cross_url.spec() + "')"));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  // No interstitial should be shown, since top-level requests are never
  // third-party.
  ASSERT_FALSE(IsShowingInterstitial());

  // The default "blocked by an extension" interstitial also should not be
  // shown. This would appear if the request was blocked by the network delegate
  // helper.
  const std::string location =
      EvalJs(web_contents(), "window.location.href").ExtractString();
  ASSERT_STRNE("chrome-error://chromewebdata/", location.c_str());
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, NoInterstitialUnlessAggressive) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com in rules but allow a.com via shields, then attempt to navigate
  // to a page on a.com. This should not show an interstitial.
  BlockDomainByURL(url);
  SetCosmeticFilteringControlType(content_settings(), ControlType::ALLOW, url);
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com in rules but set a.com to default shield settings, then attempt
  // to navigate to a page on a.com. This should not show an interstitial.
  BlockDomainByURL(url);
  SetCosmeticFilteringControlType(content_settings(),
                                  ControlType::BLOCK_THIRD_PARTY, url);
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com in rules but drop shields, then attempt to navigate
  // to a page on a.com. This should not show an interstitial.
  BlockDomainByURL(url);
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);
  SetBraveShieldsEnabled(content_settings(), false /* enable */, url);
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());
}

IN_PROC_BROWSER_TEST_F(DomainBlockDisabledTest, NoInterstitial) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK, url);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should
  // still navigate normally because domain blocking has been explicitly
  // disabled in this test.
  BlockDomainByURL(url);
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // Ensure we ended up on the expected page.
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(DomainBlockTest, ProceedDoesNotAffectOtherDomains) {
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  GURL url_a = embedded_test_server()->GetURL("a.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK,
                                  url_a);
  GURL url_b = embedded_test_server()->GetURL("b.com", "/simple.html");
  SetCosmeticFilteringControlType(content_settings(), ControlType::BLOCK,
                                  url_b);

  // Navigate to a page on a.com. This should work normally.
  NavigateTo(url_a);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block a.com, then attempt to navigate to a page on a.com. This should be
  // interrupted by the domain block interstitial.
  BlockDomainByURL(url_a);
  NavigateTo(url_a);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Proceed anyway" button. This should navigate to the
  // originally requested page.
  ClickAndWaitForNavigation("primary-button");
  ASSERT_FALSE(IsShowingInterstitial());
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());

  // Navigate to a page on b.com. This should work normally.
  NavigateTo(url_b);
  ASSERT_FALSE(IsShowingInterstitial());

  // Block b.com, then attempt to navigate to a page on b.com. This should be
  // interrupted by the domain block interstitial, because "proceed anyway"
  // permission was only given to a.com and should not apply to other domains
  // in the same tab.
  BlockDomainByURL(url_b);
  NavigateTo(url_b);
  ASSERT_TRUE(IsShowingInterstitial());
}
