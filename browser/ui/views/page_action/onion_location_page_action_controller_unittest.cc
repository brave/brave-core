// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/onion_location_page_action_controller.h"

#include <memory>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/callback_helpers.h"
#include "base/scoped_observation.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/browser/ui/views/page_action/page_action_test_observer.h"
#include "brave/browser/ui/views/page_action/test_tab_interface.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/page_action/page_action_model.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/page_action_properties_provider.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace page_actions {

class OnionLocationPageActionControllerTest : public testing::Test {
 public:
  OnionLocationPageActionControllerTest() = default;
  explicit OnionLocationPageActionControllerTest(bool use_tor_profile)
      : use_tor_profile_(use_tor_profile) {}

  void SetUp() override {
    TestingProfile* tab_profile = &profile_;
    if (use_tor_profile_) {
      // The onion chip is only shown in Tor windows.
      tab_profile = TestingProfile::Builder().BuildOffTheRecord(
          &profile_, Profile::OTRProfileID::TorID());
    }
    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(tab_profile);
    tab_interface_ =
        std::make_unique<TestTabInterface>(tab_profile, base::DoNothing());
    tab_interface_->Activate();

    tab_helper_ =
        std::make_unique<tor::OnionLocationTabHelper>(*tab_interface_);

    // Upstream actions competing with the onion chip for the chip selector.
    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_,
        std::vector<actions::ActionId>{kActionShowOnionLocation,
                                       kActionShowMemorySaverChip,
                                       kActionShowCookieControls},
        properties_provider_, pinned_actions_model_.get());

    action_item_ = actions::ActionItem::Builder()
                       .SetActionId(kActionShowOnionLocation)
                       .SetVisible(true)
                       .SetEnabled(true)
                       .Build();
    action_item_subscription_ =
        page_action_controller_->CreateActionItemSubscription(
            action_item_.get());

    page_action_controller_->AddObserver(kActionShowOnionLocation,
                                         observation_);
    page_action_controller_->AddObserver(kActionShowMemorySaverChip,
                                         memory_saver_observation_);
    page_action_controller_->AddObserver(kActionShowCookieControls,
                                         cookie_controls_observation_);

    controller_ = std::make_unique<OnionLocationPageActionController>(
        *tab_interface_, *page_action_controller_);
    controller_->Init();
  }

  void TearDown() override {
    cookie_controls_observation_.Reset();
    memory_saver_observation_.Reset();
    observation_.Reset();
    controller_.reset();
    tab_helper_.reset();
    action_item_subscription_ = base::CallbackListSubscription();
    action_item_.reset();
    page_action_controller_.reset();
    tab_interface_.reset();
    pinned_actions_model_.reset();
  }

  tor::OnionLocationTabHelper* tab_helper() { return tab_helper_.get(); }

  // Re-triggers the controller's RegisterDidActivate subscription, which is
  // how it picks up onion-location state changes made directly on the tab
  // helper (in production this happens via DidFinishNavigation instead).
  void RefreshPageAction() { tab_interface_->Activate(); }

  TestTabInterface& tab_interface() { return *tab_interface_; }
  TestingProfile& profile() { return profile_; }

  OnionLocationPageActionController* controller() { return controller_.get(); }
  PageActionControllerImpl& page_action_controller() {
    return *page_action_controller_;
  }
  const PageActionTestObserver& observer() const { return observer_; }
  const PageActionTestObserver& memory_saver_observer() const {
    return memory_saver_observer_;
  }
  const PageActionTestObserver& cookie_controls_observer() const {
    return cookie_controls_observer_;
  }

 private:
  const bool use_tor_profile_ = false;
  content::BrowserTaskEnvironment task_environment_;
  // The production provider, so chip priorities come from the patched table.
  PageActionPropertiesProvider properties_provider_;
  TestingProfile profile_;

  std::unique_ptr<PinnedToolbarActionsModel> pinned_actions_model_;
  std::unique_ptr<TestTabInterface> tab_interface_;
  std::unique_ptr<PageActionControllerImpl> page_action_controller_;
  std::unique_ptr<actions::ActionItem> action_item_;
  base::CallbackListSubscription action_item_subscription_;

  PageActionTestObserver observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      observation_{&observer_};
  PageActionTestObserver memory_saver_observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      memory_saver_observation_{&memory_saver_observer_};
  PageActionTestObserver cookie_controls_observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      cookie_controls_observation_{&cookie_controls_observer_};

  std::unique_ptr<tor::OnionLocationTabHelper> tab_helper_;
  std::unique_ptr<OnionLocationPageActionController> controller_;
};

TEST_F(OnionLocationPageActionControllerTest, HiddenByDefault) {
  EXPECT_FALSE(observer().visible());
}

TEST_F(OnionLocationPageActionControllerTest, VisibleWithOnionLocation) {
  tab_helper()->SetOnionLocationForTesting(GURL("http://example.onion/"));
  RefreshPageAction();

  EXPECT_TRUE(observer().visible());
  EXPECT_EQ(
      observer().tooltip_text(),
      l10n_util::GetStringFUTF16(IDS_LOCATION_BAR_OPEN_IN_TOR_TOOLTIP_TEXT,
                                 u"http://example.onion/"));
}

TEST_F(OnionLocationPageActionControllerTest, HiddenAgainAfterReset) {
  tab_helper()->SetOnionLocationForTesting(GURL("http://example.onion/"));
  RefreshPageAction();
  ASSERT_TRUE(observer().visible());

  tab_helper()->SetOnionLocationForTesting(GURL());
  RefreshPageAction();

  EXPECT_FALSE(observer().visible());
}

TEST_F(OnionLocationPageActionControllerTest, HiddenWhenTorDisabled) {
  tab_helper()->SetOnionLocationForTesting(GURL("http://example.onion/"));
  RefreshPageAction();
  ASSERT_TRUE(observer().visible());

  TorProfileServiceFactory::SetTorDisabled(true);
  EXPECT_FALSE(observer().visible());

  TorProfileServiceFactory::SetTorDisabled(false);
  EXPECT_TRUE(observer().visible());
}

TEST_F(OnionLocationPageActionControllerTest, HiddenWhenIncognitoDisabled) {
  tab_helper()->SetOnionLocationForTesting(GURL("http://example.onion/"));
  RefreshPageAction();
  ASSERT_TRUE(observer().visible());

  profile().GetTestingPrefService()->SetManagedPref(
      policy::policy_prefs::kIncognitoModeAvailability,
      base::Value(
          static_cast<int>(policy::IncognitoModeAvailability::kDisabled)));
  EXPECT_FALSE(observer().visible());

  profile().GetTestingPrefService()->RemoveManagedPref(
      policy::policy_prefs::kIncognitoModeAvailability);
  EXPECT_TRUE(observer().visible());
}

// The tab's contents can be swapped out - by tab discarding, or by a shared
// pinned tab being moved to another window. The tab helper follows the swapped
// in contents and forgets the old page's onion location.
TEST_F(OnionLocationPageActionControllerTest, FollowsDiscardedContents) {
  tab_helper()->SetOnionLocationForTesting(GURL("http://example.onion/"));
  RefreshPageAction();
  ASSERT_TRUE(observer().visible());

  // The swapped in contents has no onion location, so the action hides.
  tab_interface().DiscardContents();
  EXPECT_FALSE(observer().visible());

  // The swapped in contents is the one driving the page action now.
  tab_helper()->SetOnionLocationForTesting(GURL("http://other.onion/"));
  RefreshPageAction();
  EXPECT_TRUE(observer().visible());
  EXPECT_EQ(
      observer().tooltip_text(),
      l10n_util::GetStringFUTF16(IDS_LOCATION_BAR_OPEN_IN_TOR_TOOLTIP_TEXT,
                                 u"http://other.onion/"));
}

TEST_F(OnionLocationPageActionControllerTest, ExecuteActionNoopWhenHidden) {
  ASSERT_FALSE(observer().visible());

  // No onion location has been recorded, so this returns before touching
  // TorProfileManager (which needs a full browser environment this unit test
  // doesn't set up).
  controller()->ExecuteAction();
}

class OnionLocationChipPriorityTest
    : public OnionLocationPageActionControllerTest {
 public:
  OnionLocationChipPriorityTest()
      : OnionLocationPageActionControllerTest(/*use_tor_profile=*/true) {}
};

// The onion chip outranks upstream promotions instead of being hidden by them.
TEST_F(OnionLocationChipPriorityTest, PreemptsLowerPriorityUpstreamChip) {
  page_action_controller().Show(kActionShowMemorySaverChip);
  page_action_controller().ShowSuggestionChip(kActionShowMemorySaverChip);
  ASSERT_TRUE(memory_saver_observer().should_show_suggestion_chip());

  tab_helper()->SetOnionLocationForTesting(GURL("http://example.onion/"));
  RefreshPageAction();
  EXPECT_TRUE(observer().should_show_suggestion_chip());
  EXPECT_FALSE(memory_saver_observer().should_show_suggestion_chip());

  // A promotion requested while the onion chip shows stays collapsed.
  page_action_controller().ShowSuggestionChip(kActionShowMemorySaverChip);
  EXPECT_TRUE(observer().should_show_suggestion_chip());
  EXPECT_FALSE(memory_saver_observer().should_show_suggestion_chip());

  // Once the onion chip goes away the promotion gets its turn.
  tab_helper()->SetOnionLocationForTesting(GURL());
  RefreshPageAction();
  EXPECT_FALSE(observer().should_show_suggestion_chip());
  EXPECT_TRUE(memory_saver_observer().should_show_suggestion_chip());
}

// Security/trust chips are shown together rather than competing.
TEST_F(OnionLocationChipPriorityTest, CoexistsWithSecurityChip) {
  page_action_controller().Show(kActionShowCookieControls);
  page_action_controller().ShowSuggestionChip(kActionShowCookieControls);
  ASSERT_TRUE(cookie_controls_observer().should_show_suggestion_chip());

  tab_helper()->SetOnionLocationForTesting(GURL("http://example.onion/"));
  RefreshPageAction();
  EXPECT_TRUE(observer().should_show_suggestion_chip());
  EXPECT_TRUE(cookie_controls_observer().should_show_suggestion_chip());
}

}  // namespace page_actions
