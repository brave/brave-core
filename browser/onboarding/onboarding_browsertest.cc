// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <set>

#include "base/auto_reset.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/onboarding/onboarding_tab_helper.h"
#include "brave/browser/onboarding/pref_names.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

class OnboardingBrowserTest : public InProcessBrowserTest {
 public:
  bool SetUpUserDataDirectory() override {
    if (!InProcessBrowserTest::SetUpUserDataDirectory()) {
      return false;
    }
    // Brave was installed just now, so the highlight may still be shown.
    return base::WriteFile(base::PathService::CheckedGet(chrome::DIR_USER_DATA)
                               .Append(chrome::kFirstRunSentinel),
                           "");
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Armed before any tab opens, as a tab's helper checks its first run when
    // the tab is created.
    can_highlight_callback_reset_.emplace(
        OnboardingTabHelper::SetCanHighlightCallbackForTesting(
            base::BindRepeating(&OnboardingBrowserTest::OnCanHighlight,
                                base::Unretained(this))));
    did_stop_loading_callback_reset_.emplace(
        OnboardingTabHelper::SetDidStopLoadingCallbackForTesting(
            base::BindRepeating(&OnboardingBrowserTest::OnDidStopLoading,
                                base::Unretained(this))));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override {
    can_highlight_callback_reset_.reset();
    did_stop_loading_callback_reset_.reset();
    // The tabs close after this, and would leave the sets dangling.
    can_highlight_contents_.clear();
    did_stop_loading_contents_.clear();
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  base::Time GetLastHighlightTime() {
    return g_browser_process->local_state()->GetTime(
        onboarding::prefs::kLastShieldsIconHighlightTime);
  }

  // Opens a page in a new tab, reports a blocked ad, then finishes loading
  // again with a same-document navigation, which keeps the blocked count.
  // Returns once the tab's helper has considered the highlight for that load.
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
    // Wait for the tab's helper to complete its first-run check.
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return can_highlight_contents_.contains(contents); }));

    // Only the load that follows the blocked item counts.
    did_stop_loading_contents_.erase(contents);
    brave_shields::BraveShieldsTabHelper::From(
        tabs::TabInterface::MaybeGetFromContents(contents))
        ->HandleItemBlocked(brave_shields::kAds,
                            embedded_test_server()->GetURL("/ad.js").spec());
    ASSERT_TRUE(
        content::NavigateToURL(contents, url.Resolve("#same-document")));
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return did_stop_loading_contents_.contains(contents); }));
  }

  void OnCanHighlight(content::WebContents* contents) {
    can_highlight_contents_.insert(contents);
  }

  void OnDidStopLoading(content::WebContents* contents) {
    did_stop_loading_contents_.insert(contents);
  }

 private:
  // Tabs, by contents, whose helper completed its first-run check.
  std::set<raw_ptr<content::WebContents>> can_highlight_contents_;
  // Tabs, by contents, whose helper considered the highlight when loading
  // stopped.
  std::set<raw_ptr<content::WebContents>> did_stop_loading_contents_;
  std::optional<base::AutoReset<OnboardingTabHelper::WebContentsCallback>>
      can_highlight_callback_reset_;
  std::optional<base::AutoReset<OnboardingTabHelper::WebContentsCallback>>
      did_stop_loading_callback_reset_;
};

IN_PROC_BROWSER_TEST_F(OnboardingBrowserTest, HighlightsShieldsInActiveTab) {
  ASSERT_TRUE(GetLastHighlightTime().is_null());

  // The highlight is the preference being set.
  base::test::TestFuture<void> highlight_future;
  PrefChangeRegistrar registrar;
  registrar.Init(g_browser_process->local_state());
  registrar.Add(onboarding::prefs::kLastShieldsIconHighlightTime,
                base::BindLambdaForTesting([&] {
                  if (!highlight_future.IsReady()) {
                    highlight_future.SetValue();
                  }
                }));

  ASSERT_NO_FATAL_FAILURE(
      LoadPageWithBlockedAd(WindowOpenDisposition::kNewForegroundTab));

  ASSERT_TRUE(highlight_future.Wait());
  EXPECT_FALSE(GetLastHighlightTime().is_null());
}

IN_PROC_BROWSER_TEST_F(OnboardingBrowserTest,
                       DoesNotHighlightShieldsInBackgroundTab) {
  ASSERT_NO_FATAL_FAILURE(
      LoadPageWithBlockedAd(WindowOpenDisposition::kNewBackgroundTab));

  EXPECT_TRUE(GetLastHighlightTime().is_null());
}
