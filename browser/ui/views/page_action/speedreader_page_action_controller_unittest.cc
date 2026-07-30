// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/speedreader_page_action_controller.h"

#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/page_action/page_action_model.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/test_support/fake_tab_interface.h"
#include "chrome/browser/ui/page_action/test_support/test_page_action_properties_provider.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event_constants.h"

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

class SpeedreaderPageActionControllerTest : public testing::Test {
 public:
  SpeedreaderPageActionControllerTest()
      : feature_list_(speedreader::features::kSpeedreaderFeature),
        properties_provider_(PageActionPropertiesMap{{
            kActionShowSpeedreader,
            PageActionProperties{
                .histogram_name = "Speedreader",
                .type = brave::kSpeedreaderPageActionIconType,
            },
        }}) {}

  void SetUp() override {
    profile_.GetPrefs()->SetBoolean(speedreader::kSpeedreaderEnabled, true);

    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(&profile_);
    tab_interface_ = std::make_unique<FakeTabInterface>(&profile_);
    tab_interface_->Activate();

    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_, std::vector<actions::ActionId>{kActionShowSpeedreader},
        properties_provider_, pinned_actions_model_.get());

    action_item_ = actions::ActionItem::Builder()
                       .SetActionId(kActionShowSpeedreader)
                       .SetVisible(true)
                       .SetEnabled(true)
                       .Build();
    action_item_subscription_ =
        page_action_controller_->CreateActionItemSubscription(
            action_item_.get());

    page_action_controller_->AddObserver(kActionShowSpeedreader, observation_);

    speedreader::SpeedreaderTabHelper::CreateForWebContents(
        tab_interface_->GetContents(),
        *speedreader::SpeedreaderServiceFactory::GetForBrowserContext(
            &profile_),
        /*rewriter_service=*/nullptr);

    controller_ = std::make_unique<SpeedreaderPageActionController>(
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

  speedreader::SpeedreaderTabHelper* tab_helper() {
    return speedreader::SpeedreaderTabHelper::FromWebContents(
        tab_interface_->GetContents());
  }

  SpeedreaderPageActionController* controller() { return controller_.get(); }
  const TestObserver& observer() const { return observer_; }
  PrefService* prefs() { return profile_.GetPrefs(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
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

  std::unique_ptr<SpeedreaderPageActionController> controller_;
};

TEST_F(SpeedreaderPageActionControllerTest, HiddenWhenPrefDisabled) {
  prefs()->SetBoolean(speedreader::kSpeedreaderEnabled, false);
  tab_helper()->SetDistillStateForTesting(
      speedreader::Distilled(speedreader::DistillationResult::kSuccess));

  EXPECT_FALSE(observer().visible());
}

TEST_F(SpeedreaderPageActionControllerTest, HiddenWhenNotDistillable) {
  tab_helper()->SetDistillStateForTesting(speedreader::ViewOriginal(
      speedreader::ViewOriginal::Reason::kNotDistillable,
      /*was_auto_distilled=*/false));

  EXPECT_FALSE(observer().visible());
}

TEST_F(SpeedreaderPageActionControllerTest, VisibleWhenDistillable) {
  tab_helper()->SetDistillStateForTesting(speedreader::ViewOriginal());

  EXPECT_TRUE(observer().visible());
  EXPECT_EQ(
      observer().tooltip_text(),
      l10n_util::GetStringUTF16(IDS_SPEEDREADER_ICON_TURN_ON_READER_MODE));
}

TEST_F(SpeedreaderPageActionControllerTest, VisibleWhenDistilled) {
  tab_helper()->SetDistillStateForTesting(
      speedreader::Distilled(speedreader::DistillationResult::kSuccess));

  EXPECT_TRUE(observer().visible());
  EXPECT_EQ(
      observer().tooltip_text(),
      l10n_util::GetStringUTF16(IDS_SPEEDREADER_ICON_TURN_OFF_READER_MODE));
}

TEST_F(SpeedreaderPageActionControllerTest, ExecuteActionDoesNotCrash) {
  tab_helper()->SetDistillStateForTesting(speedreader::ViewOriginal());
  ASSERT_TRUE(observer().visible());

  // Right click shows the bubble; left click processes the icon click. In
  // this unit test environment there's no real BrowserWindow/WebContents
  // rendering pipeline behind either, so this just verifies neither crashes.
  controller()->ExecuteAction(ui::EF_RIGHT_MOUSE_BUTTON);
}

}  // namespace page_actions
