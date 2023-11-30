/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/brave_actions/brave_rewards_action_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

class BraveActionsContainerTest : public InProcessBrowserTest {
 public:
  BraveActionsContainerTest() = default;
  BraveActionsContainerTest(const BraveActionsContainerTest&) = delete;
  BraveActionsContainerTest& operator=(const BraveActionsContainerTest&) =
      delete;
  ~BraveActionsContainerTest() override = default;

  BraveActionsContainer* GetBraveActionsContainer(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    BraveLocationBarView* brave_location_bar_view =
        static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
    CHECK(brave_location_bar_view->brave_actions_);
    return brave_location_bar_view->brave_actions_;
  }

  void CheckBraveRewardsActionShown(Browser* browser, bool expected_shown) {
    const bool shown =
        GetBraveActionsContainer(browser)->rewards_action_btn_->GetVisible();
    ASSERT_EQ(shown, expected_shown);
  }
};

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, HideBraveRewardsAction) {
  // By default the action should be shown.
  EXPECT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kShowLocationBarButton));
  CheckBraveRewardsActionShown(browser(), true);

  // Set to hide.
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_rewards::prefs::kShowLocationBarButton, false);
  CheckBraveRewardsActionShown(browser(), false);

  // Set to show.
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_rewards::prefs::kShowLocationBarButton, true);
  CheckBraveRewardsActionShown(browser(), true);
}

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest,
                       BraveRewardsActionHiddenInGuestSession) {
  // By default the action should be shown.
  EXPECT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kShowLocationBarButton));
  CheckBraveRewardsActionShown(browser(), true);

  // Open a Guest window.
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::SwitchToGuestProfile(base::DoNothing());
  base::RunLoop().RunUntilIdle();
  browser_creation_observer.Wait();
  EXPECT_EQ(2U, BrowserList::GetInstance()->size());

  // Retrieve the new Guest profile.
  Profile* guest = g_browser_process->profile_manager()->GetProfileByPath(
      ProfileManager::GetGuestProfilePath());
  // The BrowsingDataRemover needs a loaded TemplateUrlService or else it hangs
  // on to a CallbackList::Subscription forever.
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(guest));

  // Access the browser with the Guest profile and re-init test for it.
  Browser* browser = chrome::FindAnyBrowser(guest, true);
  EXPECT_TRUE(browser);
  CheckBraveRewardsActionShown(browser, false);
}

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, ShowRewardsIconForPanel) {
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_rewards::prefs::kShowLocationBarButton, false);
  CheckBraveRewardsActionShown(browser(), false);

  // Send a request to open the Rewards panel.
  auto* coordinator =
      brave_rewards::RewardsPanelCoordinator::FromBrowser(browser());

  ASSERT_TRUE(coordinator);
  coordinator->OpenRewardsPanel();
  base::RunLoop().RunUntilIdle();

  CheckBraveRewardsActionShown(browser(), false);
}
