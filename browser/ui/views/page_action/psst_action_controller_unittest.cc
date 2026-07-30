// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/psst_action_controller.h"

#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/psst/core/common/features.h"
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
#include "ui/events/event_constants.h"
#include "ui/menus/simple_menu_model.h"

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

class TestDelegate : public PsstActionController::Delegate {
 public:
  TestDelegate() = default;
  ~TestDelegate() = default;

  // PsstActionController::Delegate:
  void OnShowConsentDialogSelected() override {
    show_consent_dialog_called_ = true;
  }
  void OnDontShowThisSiteSelected() override {
    dont_show_this_site_called_ = true;
  }
  void OnDisablePrivacySettingsTuningSelected() override {
    disable_privacy_settings_tuning_called_ = true;
  }

  bool show_consent_dialog_called_ = false;
  bool dont_show_this_site_called_ = false;
  bool disable_privacy_settings_tuning_called_ = false;
};

}  // namespace

class PsstActionControllerTest : public testing::Test {
 public:
  PsstActionControllerTest()
      : feature_list_(psst::features::kEnablePsst),
        properties_provider_(PageActionPropertiesMap{{
            kActionShowPsstIcon,
            PageActionProperties{
                .histogram_name = "PsstIcon",
                .type = brave::kPsstIconActionIconType,
            },
        }}) {}

  void SetUp() override {
    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(&profile_);
    tab_interface_ = std::make_unique<FakeTabInterface>(&profile_);
    tab_interface_->Activate();

    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_, std::vector<actions::ActionId>{kActionShowPsstIcon},
        properties_provider_, pinned_actions_model_.get());

    action_item_ = actions::ActionItem::Builder()
                       .SetActionId(kActionShowPsstIcon)
                       .SetVisible(true)
                       .SetEnabled(true)
                       .Build();
    action_item_subscription_ =
        page_action_controller_->CreateActionItemSubscription(
            action_item_.get());

    page_action_controller_->AddObserver(kActionShowPsstIcon, observation_);

    controller_ = std::make_unique<PsstActionController>(
        *tab_interface_, *page_action_controller_);
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

  PsstActionController* controller() { return controller_.get(); }
  const TestObserver& observer() const { return observer_; }

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

  std::unique_ptr<PsstActionController> controller_;
};

TEST_F(PsstActionControllerTest, HiddenByDefault) {
  EXPECT_FALSE(observer().visible());
}

TEST_F(PsstActionControllerTest, VisibleAfterSetVisibleTrue) {
  controller()->SetVisible(true);

  EXPECT_TRUE(observer().visible());
  EXPECT_EQ(observer().tooltip_text(),
            l10n_util::GetStringUTF16(IDS_IDC_PSST_LOCATION_BAR_BTN_TOOLTIP));
}

TEST_F(PsstActionControllerTest, HiddenAfterSetVisibleFalse) {
  controller()->SetVisible(true);
  ASSERT_TRUE(observer().visible());

  controller()->SetVisible(false);
  EXPECT_FALSE(observer().visible());
}

TEST_F(PsstActionControllerTest, LeftClickShowsConsentDialog) {
  TestDelegate delegate;
  controller()->SetMenuModelDelegate(&delegate);

  // Left click is handled before the (possibly null) ToolbarButtonProvider is
  // ever touched, so passing nullptr here is safe.
  controller()->ExecuteAction(/*toolbar_button_provider=*/nullptr,
                              /*item=*/nullptr, ui::EF_LEFT_MOUSE_BUTTON);

  EXPECT_TRUE(delegate.show_consent_dialog_called_);
  EXPECT_FALSE(delegate.dont_show_this_site_called_);
  EXPECT_FALSE(delegate.disable_privacy_settings_tuning_called_);
}

TEST_F(PsstActionControllerTest, MenuCommandsNotifyDelegate) {
  TestDelegate delegate;
  controller()->SetMenuModelDelegate(&delegate);

  // PsstActionController privately overrides ui::SimpleMenuModel::Delegate;
  // the methods are still reachable (and meant to be called) through the
  // base class, exactly as the real SimpleMenuModel does internally.
  auto* menu_delegate =
      static_cast<ui::SimpleMenuModel::Delegate*>(controller());

  menu_delegate->ExecuteCommand(IDC_PSST_DONT_SHOW_FOR_THIS_SITE,
                                /*event_flags=*/0);
  EXPECT_TRUE(delegate.dont_show_this_site_called_);

  menu_delegate->ExecuteCommand(IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING,
                                /*event_flags=*/0);
  EXPECT_TRUE(delegate.disable_privacy_settings_tuning_called_);
}

}  // namespace page_actions
