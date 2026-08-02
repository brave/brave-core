// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/onion_location_page_action_controller.h"

#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/page_action/page_action_model.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/test_support/fake_tab_interface.h"
#include "chrome/browser/ui/page_action/test_support/test_page_action_properties_provider.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace page_actions {

namespace {

// Records the most recent state pushed into the action's PageActionModel.
class TestObserver : public PageActionModelObserver {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  // PageActionModelObserver:
  void OnPageActionModelChanged(
      const PageActionModelInterface& model) override {
    visible_ = model.GetVisible();
    tooltip_text_ = model.GetTooltipText();
    ++model_change_count_;
  }

  bool visible() const { return visible_; }
  const std::u16string& tooltip_text() const { return tooltip_text_; }
  int model_change_count() const { return model_change_count_; }

 private:
  bool visible_ = false;
  std::u16string tooltip_text_;
  int model_change_count_ = 0;
};

}  // namespace

class OnionLocationPageActionControllerTest : public testing::Test {
 public:
  OnionLocationPageActionControllerTest()
      : properties_provider_(PageActionPropertiesMap{{
            kActionShowOnionLocation,
            PageActionProperties{
                .histogram_name = "OnionLocation",
                .type = brave::kOnionLocationPageActionIconType,
            },
        }}) {}

  void SetUp() override {
    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(&profile_);
    tab_interface_ = std::make_unique<FakeTabInterface>(&profile_);
    tab_interface_->Activate();

    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_,
        std::vector<actions::ActionId>{kActionShowOnionLocation},
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

    tor::OnionLocationTabHelper::CreateForWebContents(
        tab_interface_->GetContents());

    controller_ = std::make_unique<OnionLocationPageActionController>(
        *tab_interface_, *page_action_controller_);
    controller_->Init();
  }

  void TearDown() override {
    observation_.Reset();
    controller_.reset();
    action_item_subscription_ = base::CallbackListSubscription();
    action_item_.reset();
    page_action_controller_.reset();
    tab_interface_.reset();
    pinned_actions_model_.reset();
  }

  tor::OnionLocationTabHelper* tab_helper() {
    return tor::OnionLocationTabHelper::FromWebContents(
        tab_interface_->GetContents());
  }

  // Re-triggers the controller's RegisterDidActivate subscription, which is
  // how it picks up onion-location state changes made directly on the tab
  // helper (in production this happens via DidFinishNavigation instead).
  void RefreshPageAction() { tab_interface_->Activate(); }

  OnionLocationPageActionController* controller() { return controller_.get(); }
  const TestObserver& observer() const { return observer_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestPageActionPropertiesProvider properties_provider_;
  TestingProfile profile_;

  std::unique_ptr<PinnedToolbarActionsModel> pinned_actions_model_;
  std::unique_ptr<FakeTabInterface> tab_interface_;
  std::unique_ptr<PageActionControllerImpl> page_action_controller_;
  std::unique_ptr<actions::ActionItem> action_item_;
  base::CallbackListSubscription action_item_subscription_;

  TestObserver observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      observation_{&observer_};

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

TEST_F(OnionLocationPageActionControllerTest, ExecuteActionNoopWhenHidden) {
  ASSERT_FALSE(observer().visible());

  // No onion location has been recorded, so this returns before touching
  // TorProfileManager (which needs a full browser environment this unit test
  // doesn't set up).
  controller()->ExecuteAction();
}

}  // namespace page_actions
