/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/features.h"
#include "brave/components/unstoppable_domains/pref_names.h"
#include "brave/components/unstoppable_domains/unstoppable_domains_opt_in_page.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

security_interstitials::SecurityInterstitialPage* GetCurrentInterstitial(
    content::WebContents* web_contents) {
  security_interstitials::SecurityInterstitialTabHelper* helper =
      security_interstitials::SecurityInterstitialTabHelper::FromWebContents(
          web_contents);
  if (!helper) {
    return nullptr;
  }
  return helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting();
}

security_interstitials::SecurityInterstitialPage::TypeID GetInterstitialType(
    content::WebContents* web_contents) {
  security_interstitials::SecurityInterstitialPage* page =
      GetCurrentInterstitial(web_contents);
  if (!page) {
    return nullptr;
  }
  return page->GetTypeForTesting();
}

void SendInterstitialCommand(
    content::WebContents* web_contents,
    security_interstitials::SecurityInterstitialCommand command) {
  GetCurrentInterstitial(web_contents)
      ->CommandReceived(base::NumberToString(command));
}

void SendInterstitialCommandSync(
    Browser* browser,
    security_interstitials::SecurityInterstitialCommand command) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  EXPECT_EQ(unstoppable_domains::UnstoppableDomainsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  content::TestNavigationObserver navigation_observer(web_contents, 1);
  SendInterstitialCommand(web_contents, command);
  navigation_observer.Wait();

  EXPECT_EQ(nullptr, GetCurrentInterstitial(web_contents));
}

}  // namespace

namespace unstoppable_domains {

class UnstoppableDomainsNavigationThrottleBrowserTest
    : public InProcessBrowserTest {
 public:
  UnstoppableDomainsNavigationThrottleBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kUnstoppableDomains);
  }

  ~UnstoppableDomainsNavigationThrottleBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
  }

  PrefService* local_state() { return g_browser_process->local_state(); }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(UnstoppableDomainsNavigationThrottleBrowserTest,
                       ShowInterstitialAndProceed) {
  ui_test_utils::NavigateToURL(browser(), GURL("http://test.crypto"));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetMainFrame()));
  EXPECT_EQ(UnstoppableDomainsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS),
            local_state()->GetInteger(kResolveMethod));
}

IN_PROC_BROWSER_TEST_F(UnstoppableDomainsNavigationThrottleBrowserTest,
                       ShowInterstitialAndReject) {
  ui_test_utils::NavigateToURL(browser(), GURL("http://test.crypto"));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetMainFrame()));
  EXPECT_EQ(UnstoppableDomainsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_DONT_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::DISABLED),
            local_state()->GetInteger(kResolveMethod));
}

}  // namespace unstoppable_domains
