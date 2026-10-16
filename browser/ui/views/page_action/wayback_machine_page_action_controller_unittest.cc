// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/wayback_machine_page_action_controller.h"

#include <memory>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/page_action/page_action_test_observer.h"
#include "brave/browser/ui/views/page_action/test_tab_interface.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/page_action/page_action_model.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/test_support/test_page_action_properties_provider.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/actions/actions.h"

namespace page_actions {

class WaybackMachinePageActionControllerTest : public testing::Test {
 public:
  WaybackMachinePageActionControllerTest()
      : properties_provider_(PageActionPropertiesMap{{
            kActionShowWaybackMachine,
            PageActionProperties{
                .histogram_name = "WaybackMachine",
                .type = brave::kWaybackMachineActionIconType,
            },
        }}) {}

  void SetUp() override {
    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(&profile_);
    tab_interface_ =
        std::make_unique<TestTabInterface>(&profile_, base::DoNothing());
    tab_interface_->Activate();

    // Like BraveTabFeatures, rebuild the tab helper for swapped in contents
    // before the controller handles the swap.
    tab_helper_ = std::make_unique<BraveWaybackMachineTabHelper>(
        *tab_interface_, tab_interface_->GetContents());
    will_discard_contents_subscription_ =
        tab_interface_->RegisterWillDiscardContents(base::BindRepeating(
            [](WaybackMachinePageActionControllerTest* self,
               tabs::TabInterface* tab, content::WebContents*,
               content::WebContents* new_contents) {
              self->tab_helper_.reset();
              self->tab_helper_ =
                  std::make_unique<BraveWaybackMachineTabHelper>(*tab,
                                                                 new_contents);
            },
            base::Unretained(this)));

    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_,
        std::vector<actions::ActionId>{kActionShowWaybackMachine},
        properties_provider_, pinned_actions_model_.get());

    action_item_ = actions::ActionItem::Builder()
                       .SetActionId(kActionShowWaybackMachine)
                       .SetVisible(true)
                       .SetEnabled(true)
                       .Build();
    action_item_subscription_ =
        page_action_controller_->CreateActionItemSubscription(
            action_item_.get());

    page_action_controller_->AddObserver(kActionShowWaybackMachine,
                                         observation_);

    controller_ = std::make_unique<WaybackMachinePageActionController>(
        *tab_interface_, *page_action_controller_);
    controller_->Init();
  }

  void TearDown() override {
    observation_.Reset();
    controller_.reset();
    will_discard_contents_subscription_ = base::CallbackListSubscription();
    tab_helper_.reset();
    action_item_subscription_ = base::CallbackListSubscription();
    action_item_.reset();
    page_action_controller_.reset();
    tab_interface_.reset();
    pinned_actions_model_.reset();
  }

  content::WebContents* contents() { return tab_interface_->GetContents(); }

  BraveWaybackMachineTabHelper* tab_helper() { return tab_helper_.get(); }

  TestTabInterface& tab_interface() { return *tab_interface_; }

  const PageActionTestObserver& observer() const { return observer_; }
  PrefService* prefs() { return profile_.GetPrefs(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestPageActionPropertiesProvider properties_provider_;
  TestingProfile profile_;

  std::unique_ptr<PinnedToolbarActionsModel> pinned_actions_model_;
  std::unique_ptr<TestTabInterface> tab_interface_;
  std::unique_ptr<PageActionControllerImpl> page_action_controller_;
  std::unique_ptr<actions::ActionItem> action_item_;
  base::CallbackListSubscription action_item_subscription_;

  PageActionTestObserver observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      observation_{&observer_};

  std::unique_ptr<BraveWaybackMachineTabHelper> tab_helper_;
  base::CallbackListSubscription will_discard_contents_subscription_;
  std::unique_ptr<WaybackMachinePageActionController> controller_;
};

TEST_F(WaybackMachinePageActionControllerTest, HiddenWhenDisabledByPolicy) {
  prefs()->SetBoolean(kBraveWaybackMachineEnabled, false);
  tab_helper()->SetWaybackStateForTesting(WaybackState::kNeedToCheck);

  EXPECT_FALSE(observer().visible());
}

TEST_F(WaybackMachinePageActionControllerTest, HiddenWhenInitial) {
  tab_helper()->SetWaybackStateForTesting(WaybackState::kInitial);

  EXPECT_FALSE(observer().visible());
}

TEST_F(WaybackMachinePageActionControllerTest, VisibleWhenNeedToCheck) {
  tab_helper()->SetWaybackStateForTesting(WaybackState::kNeedToCheck);

  EXPECT_TRUE(observer().visible());
}

TEST_F(WaybackMachinePageActionControllerTest, VisibleWhenFetching) {
  tab_helper()->SetWaybackStateForTesting(WaybackState::kFetching);

  EXPECT_TRUE(observer().visible());
}

TEST_F(WaybackMachinePageActionControllerTest, HiddenWhenLoaded) {
  tab_helper()->SetWaybackStateForTesting(WaybackState::kLoaded);

  EXPECT_FALSE(observer().visible());
}

TEST_F(WaybackMachinePageActionControllerTest, VisibleWhenNotAvailable) {
  tab_helper()->SetWaybackStateForTesting(WaybackState::kNotAvailable);

  EXPECT_TRUE(observer().visible());
}

TEST_F(WaybackMachinePageActionControllerTest, HiddenAgainAfterReset) {
  tab_helper()->SetWaybackStateForTesting(WaybackState::kNotAvailable);
  ASSERT_TRUE(observer().visible());

  tab_helper()->SetWaybackStateForTesting(WaybackState::kInitial);
  EXPECT_FALSE(observer().visible());
}

// The tab's contents can be swapped out - by tab discarding, or by a shared
// pinned tab being moved to another window. The controller has to follow the
// tab helper rebuilt for the swapped in contents.
TEST_F(WaybackMachinePageActionControllerTest, FollowsDiscardedContents) {
  tab_helper()->SetWaybackStateForTesting(WaybackState::kNotAvailable);
  ASSERT_TRUE(observer().visible());

  // The rebuilt tab helper starts over, so the action hides.
  content::WebContents* const discarded_contents = contents();
  tab_interface().DiscardContents();
  ASSERT_NE(discarded_contents, contents());
  EXPECT_FALSE(observer().visible());

  // The rebuilt tab helper drives the page action.
  tab_helper()->SetWaybackStateForTesting(WaybackState::kNotAvailable);
  EXPECT_TRUE(observer().visible());
}

}  // namespace page_actions
