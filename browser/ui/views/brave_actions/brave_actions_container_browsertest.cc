/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"

#include <string>

#include "base/check_deref.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/views/brave_actions/brave_rewards_action_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_state.mojom.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_source.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "url/gurl.h"

using brave_rewards::RewardsTabHelper;

static_assert(BUILDFLAG(ENABLE_BRAVE_REWARDS));

class BraveActionsContainerTest : public InProcessBrowserTest {
 public:
  BraveActionsContainerTest() = default;
  BraveActionsContainerTest(const BraveActionsContainerTest&) = delete;
  BraveActionsContainerTest& operator=(const BraveActionsContainerTest&) =
      delete;
  ~BraveActionsContainerTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    provider_.SetDefaultReturns(
        /*is_initialization_complete_return=*/true,
        /*is_first_policy_load_complete_return=*/true);
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  void SetUpOnMainThread() override { Init(browser()); }

  void BlockRewardsByPolicy(bool value) {
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveRewardsDisabled,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_PLATFORM, base::Value(value), nullptr);
    provider_.UpdateChromePolicy(policies);
    EXPECT_EQ(
        prefs_->IsManagedPreference(brave_rewards::prefs::kDisabledByPolicy) &&
            prefs_->GetBoolean(brave_rewards::prefs::kDisabledByPolicy),
        value);
  }

  void Init(BrowserWindowInterface* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    ASSERT_NE(browser_view, nullptr);
    BraveLocationBarView* brave_location_bar_view =
        static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
    ASSERT_NE(brave_location_bar_view, nullptr);
    brave_actions_ = brave_location_bar_view->brave_actions_;
    ASSERT_NE(brave_actions_, nullptr);
    prefs_ = browser->GetProfile()->GetPrefs();
  }

  void CheckBraveRewardsActionShown(bool expected_shown) {
    const bool shown = brave_actions_->rewards_action_btn_->GetVisible();
    ASSERT_EQ(shown, expected_shown);
  }

  void CloseRewardsPanel() {
    brave_actions_->rewards_action_btn_->ClosePanelForTesting();
  }

  // The publisher the Rewards button in `browser` shows.
  static const std::string& GetShownPublisherId(
      BrowserWindowInterface* browser) {
    auto* location_bar = static_cast<BraveLocationBarView*>(
        BrowserView::GetBrowserViewForBrowser(browser)->GetLocationBarView());
    return location_bar->brave_actions_->rewards_action_btn_
        ->publisher_registered_.first;
  }

  // Opens a foreground tab and returns it.
  tabs::TabInterface* AddForegroundTab() {
    EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
        browser(), GURL("about:blank"),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
    return browser()->tab_strip_model()->GetActiveTab();
  }

  static RewardsTabHelper& GetTabHelper(tabs::TabInterface* tab) {
    return CHECK_DEREF(RewardsTabHelper::From(tab));
  }

 protected:
  raw_ptr<BraveActionsContainer, DanglingUntriaged> brave_actions_ = nullptr;
  raw_ptr<PrefService, DanglingUntriaged> prefs_ = nullptr;
  policy::MockConfigurationPolicyProvider provider_;
};

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, HideBraveRewardsAction) {
  // By default the action should be shown.
  EXPECT_TRUE(prefs_->GetBoolean(brave_rewards::prefs::kShowLocationBarButton));
  CheckBraveRewardsActionShown(true);

  // Set to hide.
  prefs_->SetBoolean(brave_rewards::prefs::kShowLocationBarButton, false);
  CheckBraveRewardsActionShown(false);

  // Set to show.
  prefs_->SetBoolean(brave_rewards::prefs::kShowLocationBarButton, true);
  CheckBraveRewardsActionShown(true);
}

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest,
                       HideBraveRewardsActionWhenDisabledByPolicy) {
  // By default the action should be shown.
  EXPECT_TRUE(prefs_->GetBoolean(brave_rewards::prefs::kShowLocationBarButton));
  CheckBraveRewardsActionShown(true);

  // Disabling Rewards via policy after the container was already
  // constructed should hide the button.
  BlockRewardsByPolicy(true);
  CheckBraveRewardsActionShown(false);

  // Lifting the policy should bring the button back.
  BlockRewardsByPolicy(false);
  CheckBraveRewardsActionShown(true);
}

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest,
                       BraveRewardsActionHiddenInGuestSession) {
  // By default the action should be shown.
  EXPECT_TRUE(prefs_->GetBoolean(brave_rewards::prefs::kShowLocationBarButton));
  CheckBraveRewardsActionShown(true);

  // Open a Guest window.
  EXPECT_EQ(1U, GlobalBrowserCollection::GetInstance()->GetSize());
  ui_test_utils::BrowserCreatedObserver browser_creation_observer;
  profiles::SwitchToGuestProfile(base::DoNothing());
  base::RunLoop().RunUntilIdle();
  browser_creation_observer.Wait();
  EXPECT_EQ(2U, GlobalBrowserCollection::GetInstance()->GetSize());

  // Retrieve the new Guest profile.
  Profile* guest = g_browser_process->profile_manager()->GetProfileByPath(
      ProfileManager::GetGuestProfilePath());
  // The BrowsingDataRemover needs a loaded TemplateUrlService or else it hangs
  // on to a CallbackList::Subscription forever.
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(guest));

  // Access the browser with the Guest profile and re-init test for it.
  BrowserWindowInterface* browser = ui_test_utils::FindAnyBrowser(guest, true);
  EXPECT_TRUE(browser);
  Init(browser);
  CheckBraveRewardsActionShown(false);
}

IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, ShowRewardsIconForPanel) {
  prefs_->SetBoolean(brave_rewards::prefs::kShowLocationBarButton, false);
  CheckBraveRewardsActionShown(false);

  // Send a request to open the Rewards panel.
  auto* coordinator = brave_rewards::RewardsPanelCoordinator::From(browser());
  ASSERT_TRUE(coordinator);
  coordinator->OpenRewardsPanel();
  base::RunLoop().RunUntilIdle();

  CheckBraveRewardsActionShown(false);
}

// The button shows the active tab's publisher, and only that tab's updates
// reach it.
IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, FollowsActiveTab) {
  TabStripModel* const tab_strip_model = browser()->tab_strip_model();
  tabs::TabInterface* const first_tab = tab_strip_model->GetActiveTab();
  GetTabHelper(first_tab).SetPublisherIdForTab("first.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "first.com");

  tabs::TabInterface* const second_tab = AddForegroundTab();
  EXPECT_EQ(GetShownPublisherId(browser()), "");
  GetTabHelper(second_tab).SetPublisherIdForTab("second.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "second.com");

  GetTabHelper(first_tab).SetPublisherIdForTab("first-updated.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "second.com");

  tab_strip_model->ActivateTabAt(tab_strip_model->GetIndexOfTab(first_tab));
  EXPECT_EQ(GetShownPublisherId(browser()), "first-updated.com");

  GetTabHelper(second_tab).SetPublisherIdForTab("second-updated.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "first-updated.com");
}

// Closing the active tab destroys its helper before the tab strip reports the
// change, and the button moves to the newly active tab.
IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, FollowsActiveTabAfterClose) {
  TabStripModel* const tab_strip_model = browser()->tab_strip_model();
  tabs::TabInterface* const first_tab = tab_strip_model->GetActiveTab();
  GetTabHelper(first_tab).SetPublisherIdForTab("first.com");

  tabs::TabInterface* const second_tab = AddForegroundTab();
  GetTabHelper(second_tab).SetPublisherIdForTab("second.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "second.com");

  tab_strip_model->CloseWebContentsAt(
      tab_strip_model->GetIndexOfTab(second_tab),
      TabCloseTypes::CLOSE_USER_GESTURE);
  ASSERT_EQ(tab_strip_model->GetActiveTab(), first_tab);
  EXPECT_EQ(GetShownPublisherId(browser()), "first.com");

  GetTabHelper(first_tab).SetPublisherIdForTab("first-updated.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "first-updated.com");
}

// A tab moved to another window keeps its helper, which then only updates the
// button of the window it moved to.
IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, FollowsTabMovedToNewWindow) {
  TabStripModel* const tab_strip_model = browser()->tab_strip_model();
  tabs::TabInterface* const first_tab = tab_strip_model->GetActiveTab();
  GetTabHelper(first_tab).SetPublisherIdForTab("first.com");

  tabs::TabInterface* const moved_tab = AddForegroundTab();
  RewardsTabHelper* const moved_tab_helper = &GetTabHelper(moved_tab);
  moved_tab_helper->SetPublisherIdForTab("moved.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "moved.com");

  chrome::MoveTabsToNewWindow(browser(),
                              {tab_strip_model->GetIndexOfTab(moved_tab)});
  BrowserWindowInterface* const new_browser =
      moved_tab->GetBrowserWindowInterface();
  ASSERT_NE(new_browser, browser());
  ASSERT_EQ(&GetTabHelper(moved_tab), moved_tab_helper);
  EXPECT_EQ(GetShownPublisherId(browser()), "first.com");
  EXPECT_EQ(GetShownPublisherId(new_browser), "moved.com");

  moved_tab_helper->SetPublisherIdForTab("moved-updated.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "first.com");
  EXPECT_EQ(GetShownPublisherId(new_browser), "moved-updated.com");
}

// A discarded tab keeps its helper, which updates the button once the tab is
// active again.
IN_PROC_BROWSER_TEST_F(BraveActionsContainerTest, FollowsDiscardedTab) {
  TabStripModel* const tab_strip_model = browser()->tab_strip_model();
  tabs::TabInterface* const first_tab = tab_strip_model->GetActiveTab();
  tabs::TabInterface* const second_tab = AddForegroundTab();
  RewardsTabHelper* const second_tab_helper = &GetTabHelper(second_tab);
  tab_strip_model->ActivateTabAt(tab_strip_model->GetIndexOfTab(first_tab));
  GetTabHelper(first_tab).SetPublisherIdForTab("first.com");

  auto* const lifecycle_unit =
      resource_coordinator::TabLifecycleUnitSource::GetTabLifecycleUnitExternal(
          second_tab->GetContents());
  ASSERT_TRUE(lifecycle_unit);
  ASSERT_TRUE(
      lifecycle_unit->DiscardTab(::mojom::LifecycleUnitDiscardReason::URGENT));
  ASSERT_EQ(&GetTabHelper(second_tab), second_tab_helper);
  EXPECT_EQ(GetShownPublisherId(browser()), "first.com");

  content::TestNavigationObserver navigation_observer(
      second_tab->GetContents());
  tab_strip_model->ActivateTabAt(tab_strip_model->GetIndexOfTab(second_tab));
  second_tab->GetContents()->GetController().LoadIfNecessary();
  navigation_observer.Wait();
  second_tab_helper->SetPublisherIdForTab("second.com");
  EXPECT_EQ(GetShownPublisherId(browser()), "second.com");
}
