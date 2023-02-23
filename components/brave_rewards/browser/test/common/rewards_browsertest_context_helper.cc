/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/bind.h"
#include "brave/browser/brave_rewards/rewards_panel/rewards_panel_coordinator.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rewards_browsertest {

using brave_rewards::RewardsPanelCoordinator;

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

  rewards_browsertest_util::WaitForElementToAppear(
      popup_contents_.get(), "[data-test-id=rewards-panel]");

  return popup_contents_;
}

base::WeakPtr<content::WebContents>
RewardsBrowserTestContextHelper::OpenSiteBanner(
    rewards_browsertest_util::TipAction tip_action) {
  base::WeakPtr<content::WebContents> popup_contents = OpenRewardsPopup();

  // Construct an observer to wait for the site banner to load.
  content::CreateAndLoadWebContentsObserver site_banner_observer;

  std::string button_selector;
  bool open_tip_actions = false;

  switch (tip_action) {
    case rewards_browsertest_util::TipAction::OneTime:
      button_selector = "[data-test-id=tip-button]";
      break;
    case rewards_browsertest_util::TipAction::SetMonthly:
      button_selector = "[data-test-id=set-monthly-tip-button]";
      break;
    case rewards_browsertest_util::TipAction::ChangeMonthly:
      button_selector = "[data-test-id=change-monthly-tip-button]";
      open_tip_actions = true;
      break;
    case rewards_browsertest_util::TipAction::ClearMonthly:
      button_selector = "[data-test-id=cancel-monthly-tip-button]";
      open_tip_actions = true;
      break;
  }

  // If necessary, show the monthly tip actions menu.
  if (open_tip_actions) {
    rewards_browsertest_util::WaitForElementThenClick(
        popup_contents.get(), "[data-test-id=monthly-tip-actions-button]");
  }

  // Click button to initiate sending a tip.
  rewards_browsertest_util::WaitForElementThenClick(popup_contents.get(),
                                                    button_selector);

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

void RewardsBrowserTestContextHelper::VisitPublisher(
    const GURL& url,
    const bool verified,
    const bool last_add) {
  const std::string publisher = url.host();
  ui_test_utils::NavigateToURLWithDisposition(
      browser_,
      url,
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // The minimum publisher duration when testing is 1 second (and the
  // granularity is seconds), so wait for just over 2 seconds to elapse
  base::PlatformThread::Sleep(base::Milliseconds(2100));

  LoadRewardsPage();

  auto* contents = browser_->tab_strip_model()->GetActiveWebContents();

  // Ensure that we are on the Rewards page.
  EXPECT_EQ(contents->GetLastCommittedURL().host_piece(), "rewards");

  // Ensure that the AC box is displayed.
  rewards_browsertest_util::WaitForElementToAppear(
      contents, "[data-test-id=auto-contribute-panel]");

  // Ensure that the AC sites table is displayed.
  rewards_browsertest_util::WaitForElementToAppear(
      contents, "[data-test-id=auto-contribute-table]");

  // Make sure site appears in auto-contribute table
  rewards_browsertest_util::WaitForElementToEqual(
      contents,
      "[data-test-id='ac_link_" + publisher + "']",
      publisher);

  if (verified) {
    // A verified site has two images associated with it, the site's
    // favicon and the verified icon
    content::EvalJsResult js_result = EvalJs(
        contents,
        "document.querySelector(\"[data-test-id='ac_link_" +
        publisher + "']\").getElementsByTagName('svg').length === 1;",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_TRUE(js_result.ExtractBool());
  } else {
    // An unverified site has one image associated with it, the site's
    // favicon
    content::EvalJsResult js_result = EvalJs(
        contents,
        "document.querySelector(\"[data-test-id='ac_link_" +
        publisher + "']\").getElementsByTagName('svg').length === 0;",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_TRUE(js_result.ExtractBool());
  }
}

void RewardsBrowserTestContextHelper::LoadURL(GURL url) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser_, url));
  auto* contents = browser_->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
}

void RewardsBrowserTestContextHelper::LoadRewardsPage() {
  GURL url = rewards_browsertest_util::GetRewardsUrl();
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
  rewards_browsertest_util::WaitForElementToAppear(
      tab_strip->GetActiveWebContents(), "[data-test-id=rewards-balance-text]");
}

void RewardsBrowserTestContextHelper::ReloadCurrentSite() {
  auto* contents = browser_->tab_strip_model()->GetActiveWebContents();
  contents->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents));
}

}  // namespace rewards_browsertest
