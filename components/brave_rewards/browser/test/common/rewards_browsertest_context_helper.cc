/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/bind_test_util.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rewards_browsertest_helper {

void OpenRewardsPopupRewardsEnabled(Browser* browser) {
  // Ask the popup to open
  std::string error;
  bool popup_shown = extensions::BraveActionAPI::ShowActionUI(
    browser,
    brave_rewards_extension_id,
    nullptr,
    &error);
  if (!popup_shown) {
    LOG(ERROR) << "Could not open rewards popup: " << error;
  }
  EXPECT_TRUE(popup_shown);
}

void OpenRewardsPopupRewardsDisabled(Browser* browser) {
  BrowserView* browser_view =
    BrowserView::GetBrowserViewForBrowser(browser);
  BraveLocationBarView* brave_location_bar_view =
      static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
  ASSERT_NE(brave_location_bar_view, nullptr);
  auto* brave_actions = brave_location_bar_view->GetBraveActionsContainer();
  ASSERT_NE(brave_actions, nullptr);

  brave_actions->OnRewardsStubButtonClicked();
}

content::WebContents* OpenRewardsPopup(Browser* browser) {
  // Construct an observer to wait for the popup to load
  content::WebContents* popup_contents = nullptr;
  auto check_load_is_rewards_panel =
      [&](const content::NotificationSource& source,
          const content::NotificationDetails&) -> bool {
        auto web_contents_source =
            static_cast<const content::Source<content::WebContents>&>(source);
        popup_contents = web_contents_source.ptr();

        // Check that this notification is for the Rewards panel and not, say,
        // the extension background page.
        std::string url = popup_contents->GetLastCommittedURL().spec();
        std::string rewards_panel_url = std::string("chrome-extension://") +
            brave_rewards_extension_id + "/brave_rewards_panel.html";
        return url == rewards_panel_url;
      };

  content::WindowedNotificationObserver popup_observer(
      content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
      base::BindLambdaForTesting(check_load_is_rewards_panel));

  bool rewards_enabled = browser->profile()->GetPrefs()->
      GetBoolean(brave_rewards::prefs::kEnabled);

  if (rewards_enabled) {
    OpenRewardsPopupRewardsEnabled(browser);
  } else {
    OpenRewardsPopupRewardsDisabled(browser);
  }

  // Wait for the popup to load
  popup_observer.Wait();
  rewards_browsertest_util::WaitForElementToAppear(
      popup_contents,
      "[data-test-id='rewards-panel']");

  return popup_contents;
}

void EnableRewards(Browser* browser, const bool use_new_tab) {
  // Load rewards page
  GURL page_url = use_new_tab
      ? rewards_browsertest_util::GetNewTabUrl()
      : rewards_browsertest_util::GetRewardsUrl();
  LoadURL(browser, page_url);

  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  // Opt in and create wallet to enable rewards
  rewards_browsertest_util::WaitForElementThenClick(
      contents,
      "[data-test-id='optInAction']");
  rewards_browsertest_util::WaitForElementToAppear(
      contents,
      "[data-test-id2='enableMain']");
}

content::WebContents* OpenSiteBanner(
    Browser* browser,
    rewards_browsertest_util::ContributionType banner_type) {
  content::WebContents* popup_contents =
      rewards_browsertest_helper::OpenRewardsPopup(browser);

  // Construct an observer to wait for the site banner to load.
  content::WindowedNotificationObserver site_banner_observer(
      content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
      content::NotificationService::AllSources());

  const std::string button_selector =
      banner_type == rewards_browsertest_util::ContributionType::MonthlyTip
      ? "[type='tip-monthly']"
      : "[type='tip']";

  // Click button to initiate sending a tip.
  rewards_browsertest_util::WaitForElementThenClick(
      popup_contents,
      button_selector);

  // Wait for the site banner to load
  site_banner_observer.Wait();

  // Retrieve the notification source
  const auto& site_banner_source =
      static_cast<const content::Source<content::WebContents>&>(
          site_banner_observer.source());

  // Allow the site banner to update its UI. We cannot use ExecJs here,
  // because it does not resolve promises.
  (void)EvalJs(site_banner_source.ptr(),
      "new Promise(resolve => setTimeout(resolve, 0))",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  return site_banner_source.ptr();
}

void VisitPublisher(
    Browser* browser,
    const GURL& url,
    const bool verified,
    const bool last_add) {
  const std::string publisher = url.host();
  ui_test_utils::NavigateToURLWithDisposition(
      browser,
      url,
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // The minimum publisher duration when testing is 1 second (and the
  // granularity is seconds), so wait for just over 2 seconds to elapse
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(2100));

  // Activate the Rewards settings page tab
  rewards_browsertest_util::ActivateTabAtIndex(browser, 0);

  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
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

void LoadURL(Browser* browser, GURL url) {
  DCHECK(browser);
  ui_test_utils::NavigateToURL(browser, url);
  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
}

void ReloadCurrentSite(Browser* browser) {
  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  contents->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents));
}

}  // namespace rewards_browsertest_helper
