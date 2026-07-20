/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_delegate.h"

#include <memory>

#include "build/build_config.h"

// `AdsServiceDelegate::OpenNewTabWithUrl` only drives `Navigate()` and
// `Browser::Create()` on non-Android desktop platforms; Android uses
// `ServiceTabLauncher` instead, which is untouched by this fix.
#if !BUILDFLAG(IS_ANDROID)

#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

class AdsServiceDelegateBrowserTest : public InProcessBrowserTest {
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
          *browser()->profile(), *g_browser_process->local_state(),
          /*adaptive_captcha_service=*/nullptr);
    }

    return *delegate_;
  }

 private:
  std::unique_ptr<AdsServiceDelegate> delegate_;
};

IN_PROC_BROWSER_TEST_F(AdsServiceDelegateBrowserTest,
                       OpenNewTabWithUrlNavigatesToUrl) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");

  delegate().OpenNewTabWithUrl(url);

  content::WebContents* const web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(AdsServiceDelegateBrowserTest,
                       OpenNewTabWithUrlDoesNotNavigateAfterShutdownStarted) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  content::WebContents* const web_contents_before =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_before);
  const int tab_count_before = browser()->tab_strip_model()->count();

  browser_shutdown::OnShutdownStarting(
      browser_shutdown::ShutdownType::kWindowClose);

  delegate().OpenNewTabWithUrl(url);

  EXPECT_EQ(tab_count_before, browser()->tab_strip_model()->count());
  EXPECT_EQ(web_contents_before,
            browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_NE(url, web_contents_before->GetLastCommittedURL());

  browser_shutdown::ResetShutdownGlobalsForTesting();
}

}  // namespace brave_ads

#endif  // !BUILDFLAG(IS_ANDROID)
