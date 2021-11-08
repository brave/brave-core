/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
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
#include "extensions/common/constants.h"

class BraveActionsContainerTest : public InProcessBrowserTest {
 public:
  BraveActionsContainerTest() = default;
  BraveActionsContainerTest(const BraveActionsContainerTest&) = delete;
  BraveActionsContainerTest& operator=(const BraveActionsContainerTest&) =
      delete;
  ~BraveActionsContainerTest() override = default;

  void SetUpOnMainThread() override { Init(browser()); }

  void Init(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    ASSERT_NE(browser_view, nullptr);
    BraveLocationBarView* brave_location_bar_view =
        static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
    ASSERT_NE(brave_location_bar_view, nullptr);
    brave_actions_ = brave_location_bar_view->brave_actions_;
    ASSERT_NE(brave_actions_, nullptr);
    prefs_ = browser->profile()->GetPrefs();
  }

  void CheckBraveRewardsActionShown(bool expected_shown) {
    const bool shown =
        brave_actions_->IsActionShown(brave_rewards_extension_id);
    ASSERT_EQ(shown, expected_shown);
  }

 protected:
  BraveActionsContainer* brave_actions_;
  PrefService* prefs_;
};

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, HideBraveRewardsAction) {
  // By default the action should be shown.
  EXPECT_TRUE(prefs_->GetBoolean(brave_rewards::prefs::kShowButton));
  CheckBraveRewardsActionShown(true);

  // Set to hide.
  prefs_->SetBoolean(brave_rewards::prefs::kShowButton, false);
  CheckBraveRewardsActionShown(false);

  // Set to show.
  prefs_->SetBoolean(brave_rewards::prefs::kShowButton, true);
  CheckBraveRewardsActionShown(true);
}

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest,
                       BraveRewardsActionHiddenInGuestSession) {
  // By default the action should be shown.
  EXPECT_TRUE(prefs_->GetBoolean(brave_rewards::prefs::kShowButton));
  CheckBraveRewardsActionShown(true);

  // Open a Guest window.
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::SwitchToGuestProfile(ProfileManager::CreateCallback());
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
  Init(browser);
  CheckBraveRewardsActionShown(false);
}

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, ShowRewardsIconForPanel) {
  prefs_->SetBoolean(brave_rewards::prefs::kShowButton, false);
  CheckBraveRewardsActionShown(false);

  // Simulate pressing the "stub" button to ensure that the extension is loaded.
  brave_actions_->OnRewardsStubButtonClicked();
  base::RunLoop().RunUntilIdle();

  // Simulate an action from the brave actions API to open the rewards panel.
  extensions::BraveActionAPI::Observer* action_observer = brave_actions_;
  action_observer->OnBraveActionShouldTrigger(brave_rewards_extension_id,
                                              nullptr);

  base::RunLoop().RunUntilIdle();

  // Rewards action should be shown while popup is open.
  CheckBraveRewardsActionShown(true);

  // Close the rewards popup.
  static_cast<ExtensionsContainer*>(brave_actions_)->HideActivePopup();

  base::RunLoop().RunUntilIdle();

  // Rewards action should be hidden after popup is closed.
  CheckBraveRewardsActionShown(false);
}
