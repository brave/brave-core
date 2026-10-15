// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/location.h"
#include "base/test/run_until.h"
#include "base/time/time.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/onboarding/onboarding_tab_helper.h"
#include "brave/browser/onboarding/pref_names.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

class OnboardingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Brave was installed today, so the highlight may still be shown.
    OnboardingTabHelper::s_sentinel_time_for_testing_ = base::Time::Now();
    OnboardingTabHelper::s_time_now_for_testing_ = base::Time::Now();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override {
    OnboardingTabHelper::s_sentinel_time_for_testing_.reset();
    OnboardingTabHelper::s_time_now_for_testing_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  base::Time GetLastHighlightTime() {
    return g_browser_process->local_state()->GetTime(
        onboarding::prefs::kLastShieldsIconHighlightTime);
  }

  // Opens a page in a new tab, reports a blocked ad, then finishes loading
  // again with a same-document navigation, which keeps the blocked count.
  void LoadPageWithBlockedAd(
      WindowOpenDisposition disposition,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    const GURL url = embedded_test_server()->GetURL("/title1.html");
    content::RenderFrameHost* frame =
        ui_test_utils::NavigateToURLWithDisposition(
            browser(), url, disposition,
            ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    ASSERT_TRUE(frame);
    content::WebContents* contents =
        content::WebContents::FromRenderFrameHost(frame);
    // Let the helper's first-run check complete.
    content::RunAllTasksUntilIdle();

    brave_shields::BraveShieldsTabHelper::FromWebContents(contents)
        ->HandleItemBlocked(brave_shields::kAds,
                            embedded_test_server()->GetURL("/ad.js").spec());
    ASSERT_TRUE(
        content::NavigateToURL(contents, url.Resolve("#same-document")));
  }
};

IN_PROC_BROWSER_TEST_F(OnboardingBrowserTest, HighlightsShieldsInActiveTab) {
  ASSERT_TRUE(GetLastHighlightTime().is_null());

  ASSERT_NO_FATAL_FAILURE(
      LoadPageWithBlockedAd(WindowOpenDisposition::NEW_FOREGROUND_TAB));

  EXPECT_TRUE(
      base::test::RunUntil([&] { return !GetLastHighlightTime().is_null(); }));
}

IN_PROC_BROWSER_TEST_F(OnboardingBrowserTest,
                       DoesNotHighlightShieldsInBackgroundTab) {
  ASSERT_NO_FATAL_FAILURE(
      LoadPageWithBlockedAd(WindowOpenDisposition::NEW_BACKGROUND_TAB));
  content::RunAllTasksUntilIdle();

  EXPECT_TRUE(GetLastHighlightTime().is_null());
}
