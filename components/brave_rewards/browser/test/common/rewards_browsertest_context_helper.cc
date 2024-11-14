/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"

#include <string>

#include "base/test/bind.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::test_util {

RewardsBrowserTestContextHelper::RewardsBrowserTestContextHelper(
    Browser* browser) {
  browser_ = browser;
}

RewardsBrowserTestContextHelper::~RewardsBrowserTestContextHelper() = default;

void RewardsBrowserTestContextHelper::OpenPopup() {
  // Ask the popup to open
  auto* coordinator = RewardsPanelCoordinator::FromBrowser(browser_);
  ASSERT_TRUE(coordinator);
  bool popup_shown = coordinator->OpenRewardsPanel();
  if (!popup_shown) {
    LOG(ERROR) << "Could not open rewards popup";
  }

  EXPECT_TRUE(popup_shown);
}

base::WeakPtr<content::WebContents>
RewardsBrowserTestContextHelper::OpenRewardsPopup() {
  if (popup_contents_) {
    return popup_contents_;
  }

  content::CreateAndLoadWebContentsObserver popup_observer;

  OpenPopup();

  // Wait for the popup to load
  do {
    content::WebContents* web_contents = popup_observer.Wait();
    GURL url = web_contents->GetLastCommittedURL();
    if (RewardsPanelCoordinator::IsRewardsPanelURLForTesting(url)) {
      popup_contents_ = web_contents->GetWeakPtr();
    }
  } while (!popup_contents_);

  test_util::WaitForElementToAppear(popup_contents_.get(),
                                    "[data-test-id=rewards-panel]");

  return popup_contents_;
}

base::WeakPtr<content::WebContents>
RewardsBrowserTestContextHelper::OpenSiteBanner() {
  base::WeakPtr<content::WebContents> popup_contents = OpenRewardsPopup();

  // Construct an observer to wait for the site banner to load.
  content::CreateAndLoadWebContentsObserver site_banner_observer;

  // Click button to initiate sending a tip.
  test_util::WaitForElementThenClick(popup_contents.get(),
                                     "[data-test-id=tip-button]:enabled");

  LOG(INFO) << "Waiting for tip panel to open";

  // Wait for the site banner to load and retrieve the notification source
  base::WeakPtr<content::WebContents> banner =
      site_banner_observer.Wait()->GetWeakPtr();

  // Allow the site banner to update its UI. We cannot use ExecJs here,
  // because it does not resolve promises.
  (void)EvalJs(banner.get(), "new Promise(resolve => setTimeout(resolve, 0))",
               content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
               content::ISOLATED_WORLD_ID_CONTENT_END);

  return banner;
}

void RewardsBrowserTestContextHelper::VisitPublisher(const GURL& url,
                                                     bool verified) {
  const std::string publisher = url.host();
  ui_test_utils::NavigateToURLWithDisposition(
      browser_,
      url,
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // The minimum publisher duration when testing is 1 second (and the
  // granularity is seconds), so wait for just over 2 seconds to elapse
  test_util::WaitForAutoContributeVisitTime();

  LoadRewardsPage();
}

void RewardsBrowserTestContextHelper::LoadURL(GURL url) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser_, url));
  auto* contents = browser_->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
}

void RewardsBrowserTestContextHelper::LoadRewardsPage() {
  GURL url = test_util::GetRewardsUrl();
  auto* tab_strip = browser_->tab_strip_model();

  // Activate the rewards page if it's already loaded into a tab.
  bool found = false;
  for (int index = 0; index < tab_strip->count(); ++index) {
    auto* contents = tab_strip->GetWebContentsAt(index);
    if (contents->GetLastCommittedURL().host_piece() == url.host_piece()) {
      found = true;
      tab_strip->ActivateTabAt(index);
      break;
    }
  }

  // Otherwise, load the rewards page into a new tab.
  if (!found) {
    LoadURL(url);
  }

  // Wait for the content to be fully rendered before continuing.
  test_util::WaitForElementToAppear(tab_strip->GetActiveWebContents(),
                                    "#rewardsPage");
}

void RewardsBrowserTestContextHelper::ReloadCurrentSite() {
  auto* contents = browser_->tab_strip_model()->GetActiveWebContents();
  contents->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents));
}

}  // namespace brave_rewards::test_util
