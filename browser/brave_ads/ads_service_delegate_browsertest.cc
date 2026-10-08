/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_delegate.h"

#include <memory>

#include "build/build_config.h"

// `AdsServiceDelegate::OpenNewTabWithUrl` only drives `Navigate()` and
// `CreateBrowserWindow()` on non-Android desktop platforms; Android uses
// `ServiceTabLauncher` instead, which is untouched by this fix.
#if !BUILDFLAG(IS_ANDROID)

#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime_desktop.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_list/tab_list_interface.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

class BraveAdsServiceDelegateBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override {
    // `delegate_` holds a `raw_ref` to `g_browser_process->local_state()`,
    // which is destroyed during browser shutdown inside
    // `BrowserTestBase::SetUp()`. Release it here, while the browser process
    // is still alive, rather than letting it dangle until the fixture itself
    // is destroyed.
    delegate_.reset();

    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  AdsServiceDelegate& delegate() {
    if (!delegate_) {
      delegate_ = std::make_unique<AdsServiceDelegate>(
          *browser()->GetProfile(), *g_browser_process->local_state(),
          /*adaptive_captcha_service=*/nullptr);
    }

    return *delegate_;
  }

 private:
  std::unique_ptr<AdsServiceDelegate> delegate_;
};

IN_PROC_BROWSER_TEST_F(BraveAdsServiceDelegateBrowserTest,
                       OpenNewTabWithUrlNavigatesToUrl) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");

  delegate().OpenNewTabWithUrl(url);

  content::WebContents* const web_contents =
      chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(web_contents);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceDelegateBrowserTest,
                       OpenNewTabWithUrlDoesNotNavigateAfterShutdownStarted) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  content::WebContents* const web_contents_before =
      chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(web_contents_before);
  const int tab_count_before = GetTabListInterface()->GetTabCount();

  browser_shutdown::OnShutdownStarting(
      browser_shutdown::ShutdownType::kWindowClose);

  delegate().OpenNewTabWithUrl(url);

  EXPECT_EQ(tab_count_before, GetTabListInterface()->GetTabCount());
  EXPECT_EQ(web_contents_before, chrome_test_utils::GetActiveWebContents(this));
  EXPECT_NE(url, web_contents_before->GetLastCommittedURL());

  browser_shutdown::ResetShutdownGlobalsForTesting();
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceDelegateBrowserTest,
                       OpenNewTabWithUrlCreatesBrowserWhenNoneExists) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");

  AdsServiceDelegate& ads_service_delegate = delegate();
  Profile* const profile = browser()->GetProfile();

  // Keep the browser process and profile alive while all browser windows for
  // this profile are closed below.
  ScopedKeepAlive keep_alive(KeepAliveOrigin::BROWSER,
                             KeepAliveRestartOption::DISABLED);
  ScopedProfileKeepAlive profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kBrowserWindow);

  ui_test_utils::BrowserDestroyedObserver observer(browser());
  chrome::CloseAllBrowsersWithProfile(profile);
  observer.Wait();
  EXPECT_EQ(ProfileBrowserCollection::GetForProfile(profile)->GetSize(), 0U);

  ui_test_utils::BrowserCreatedObserver browser_created_observer;
  ads_service_delegate.OpenNewTabWithUrl(url);
  BrowserWindowInterface* const new_browser = browser_created_observer.Wait();
  ASSERT_TRUE(new_browser);
  EXPECT_EQ(ProfileBrowserCollection::GetForProfile(profile)->GetSize(), 1U);

  content::WebContents* const web_contents =
      new_browser->GetTabStripModel()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());
}

}  // namespace brave_ads

#endif  // !BUILDFLAG(IS_ANDROID)
