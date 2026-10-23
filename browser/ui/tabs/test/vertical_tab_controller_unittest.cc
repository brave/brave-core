// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/auto_reset.h"
#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/sessions/brave_session_keys.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/unowned_user_data/unowned_user_data_host.h"

class VerticalTabControllerUnitTest : public testing::Test {
 public:
  void SetUp() override {
    tabs::RegisterProfilePrefs(pref_service_.registry());
    brave_tabs::RegisterBraveProfilePrefs(pref_service_.registry());
  }

 protected:
  std::unique_ptr<VerticalTabController> MakeController(
      BrowserWindowInterface::Type type = BrowserWindowInterface::TYPE_NORMAL,
      FocusModeController* focus_mode_controller = nullptr) {
    return std::make_unique<VerticalTabController>(
        user_data_host_, nullptr, type, &pref_service_, focus_mode_controller);
  }

  // Test bodies subclass this fixture, so the private hook is reached here.
  [[nodiscard]] static base::AutoReset<std::optional<bool>>
  InheritEnabledOverrideForTesting(std::optional<bool> enabled) {
    return VerticalTabController::InheritEnabledOverrideForTesting(
        std::move(enabled));
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
  // Each test creates at most one controller of each type, which registers
  // itself here.
  ui::UnownedUserDataHost user_data_host_;
};

TEST_F(VerticalTabControllerUnitTest, SupportsBraveVerticalTabsNormalWindow) {
  auto controller = MakeController(BrowserWindowInterface::TYPE_NORMAL);
  EXPECT_TRUE(controller->SupportsBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, SupportsBraveVerticalTabsPopupWindow) {
  auto controller = MakeController(BrowserWindowInterface::TYPE_POPUP);
  EXPECT_FALSE(controller->SupportsBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, ShouldShowBraveVerticalTabsDefaultOff) {
  auto controller = MakeController();
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, ShouldShowBraveVerticalTabsWhenEnabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  auto controller = MakeController();
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest,
       ShouldShowBraveVerticalTabsFalseForPopupEvenIfPrefEnabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  auto controller = MakeController(BrowserWindowInterface::TYPE_POPUP);
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, ShouldShowWindowTitleWhenEnabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow, true);
  auto controller = MakeController();
  EXPECT_TRUE(controller->ShouldShowWindowTitleForVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest,
       ShouldShowWindowTitleFalseWhenVerticalTabsDisabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, false);
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow, true);
  auto controller = MakeController();
  EXPECT_FALSE(controller->ShouldShowWindowTitleForVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, ShouldShowWindowTitleFalseInFocusMode) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow, true);

  FocusModeController focus_mode_controller(user_data_host_);
  auto controller = MakeController(BrowserWindowInterface::TYPE_NORMAL,
                                   &focus_mode_controller);
  EXPECT_TRUE(controller->ShouldShowWindowTitleForVerticalTabs());

  focus_mode_controller.SetEnabled(true);
  EXPECT_FALSE(controller->ShouldShowWindowTitleForVerticalTabs());

  focus_mode_controller.SetEnabled(false);
  EXPECT_TRUE(controller->ShouldShowWindowTitleForVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, IsFloatingVerticalTabsEnabledDefault) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  // kVerticalTabsFloatingEnabled defaults to true
  auto controller = MakeController();
  EXPECT_TRUE(controller->IsFloatingVerticalTabsEnabled());
}

TEST_F(VerticalTabControllerUnitTest,
       IsFloatingVerticalTabsDisabledWhenVerticalTabsOff) {
  auto controller = MakeController();
  EXPECT_FALSE(controller->IsFloatingVerticalTabsEnabled());
}

TEST_F(VerticalTabControllerUnitTest,
       ShouldShowVerticalTabToggleButtonDefault) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  // kVerticalTabsShowToggleButton defaults to true
  auto controller = MakeController();
  EXPECT_TRUE(controller->ShouldShowVerticalTabToggleButton());
}

TEST_F(VerticalTabControllerUnitTest,
       ShouldShowVerticalTabToggleButtonFalseWhenVerticalTabsOff) {
  auto controller = MakeController();
  EXPECT_FALSE(controller->ShouldShowVerticalTabToggleButton());
}

TEST_F(VerticalTabControllerUnitTest,
       IsFloatingVerticalTabsEnabledWhenToggleButtonHidden) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsFloatingEnabled, false);
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsShowToggleButton, false);
  auto controller = MakeController();
  EXPECT_FALSE(controller->ShouldShowVerticalTabToggleButton());
  // Floating mode is forced on when there is no toggle button to expand
  // collapsed tabs, regardless of kVerticalTabsFloatingEnabled.
  EXPECT_TRUE(controller->IsFloatingVerticalTabsEnabled());
}

TEST_F(VerticalTabControllerUnitTest, IsVerticalTabOnRightDefault) {
  auto controller = MakeController();
  EXPECT_FALSE(controller->IsVerticalTabOnRight());
}

TEST_F(VerticalTabControllerUnitTest, WindowOverrideDoesNotAffectOtherWindow) {
  ui::UnownedUserDataHost other_host;
  auto first = MakeController();
  auto second = std::make_unique<VerticalTabController>(
      other_host, nullptr, BrowserWindowInterface::TYPE_NORMAL, &pref_service_,
      nullptr);

  first->SetVerticalTabsEnabledForWindow(true);
  EXPECT_TRUE(first->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(second->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(pref_service_.GetBoolean(brave_tabs::kVerticalTabsEnabled));

  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  // The shared setting caught up, so this window follows it again.
  EXPECT_EQ(std::nullopt, first->vertical_tabs_enabled_override());
  EXPECT_TRUE(first->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(second->ShouldShowBraveVerticalTabs());

  first->SetVerticalTabsEnabledForWindow(false);
  EXPECT_FALSE(first->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(second->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(pref_service_.GetBoolean(brave_tabs::kVerticalTabsEnabled));
}

TEST_F(VerticalTabControllerUnitTest, EnabledChangedCallbackFiresOnOverride) {
  auto controller = MakeController();
  bool notified = false;
  auto subscription = controller->AddEnabledChangedCallback(
      base::BindRepeating([](bool* notified) { *notified = true; }, &notified));

  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  EXPECT_TRUE(notified);

  notified = false;
  controller->SetVerticalTabsEnabledForWindow(false);
  EXPECT_TRUE(notified);
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());

  notified = false;
  controller->SetVerticalTabsEnabledForWindow(false);
  EXPECT_FALSE(notified);

  notified = false;
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, false);
  EXPECT_TRUE(notified);
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
}

TEST_F(VerticalTabControllerUnitTest,
       InheritedOverrideConsumedByNextNormalWindow) {
  ui::UnownedUserDataHost other_host;
  [[maybe_unused]] auto inherit = InheritEnabledOverrideForTesting(true);
  auto first = MakeController();
  EXPECT_EQ(std::optional<bool>(true), first->vertical_tabs_enabled_override());
  EXPECT_TRUE(first->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(pref_service_.GetBoolean(brave_tabs::kVerticalTabsEnabled));

  auto second = std::make_unique<VerticalTabController>(
      other_host, nullptr, BrowserWindowInterface::TYPE_NORMAL, &pref_service_,
      nullptr);
  EXPECT_EQ(std::nullopt, second->vertical_tabs_enabled_override());
  EXPECT_FALSE(second->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, PopupDoesNotConsumeInheritedOverride) {
  [[maybe_unused]] auto inherit = InheritEnabledOverrideForTesting(true);
  ui::UnownedUserDataHost popup_host;
  auto popup = std::make_unique<VerticalTabController>(
      popup_host, nullptr, BrowserWindowInterface::TYPE_POPUP, &pref_service_,
      nullptr);
  EXPECT_EQ(std::nullopt, popup->vertical_tabs_enabled_override());

  auto normal = MakeController();
  EXPECT_EQ(std::optional<bool>(true),
            normal->vertical_tabs_enabled_override());
  EXPECT_TRUE(normal->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest,
       ProfilePrefUpdatesWindowsWithoutOverride) {
  ui::UnownedUserDataHost other_host;
  auto first = MakeController();
  auto second = std::make_unique<VerticalTabController>(
      other_host, nullptr, BrowserWindowInterface::TYPE_NORMAL, &pref_service_,
      nullptr);

  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  EXPECT_TRUE(first->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(second->ShouldShowBraveVerticalTabs());
  EXPECT_EQ(std::nullopt, first->vertical_tabs_enabled_override());
  EXPECT_EQ(std::nullopt, second->vertical_tabs_enabled_override());

  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, false);
  EXPECT_FALSE(first->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(second->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, FollowProfilePrefClearsThisWindowOnly) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, false);
  auto first = MakeController();
  ui::UnownedUserDataHost other_host;
  auto second = std::make_unique<VerticalTabController>(
      other_host, nullptr, BrowserWindowInterface::TYPE_NORMAL, &pref_service_,
      nullptr);
  first->SetVerticalTabsEnabledForWindow(true);
  second->SetVerticalTabsEnabledForWindow(true);

  bool notified = false;
  auto subscription = first->AddEnabledChangedCallback(
      base::BindRepeating([](bool* notified) { *notified = true; }, &notified));

  first->FollowProfilePref();

  EXPECT_EQ(std::nullopt, first->vertical_tabs_enabled_override());
  EXPECT_FALSE(first->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(notified);
  EXPECT_EQ(std::optional<bool>(true),
            second->vertical_tabs_enabled_override());
  EXPECT_TRUE(second->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(pref_service_.GetBoolean(brave_tabs::kVerticalTabsEnabled));
}

TEST_F(VerticalTabControllerUnitTest,
       ReturningWindowToSharedSettingClearsOverride) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  auto controller = MakeController();

  controller->SetVerticalTabsEnabledForWindow(false);
  EXPECT_EQ(std::optional<bool>(false),
            controller->vertical_tabs_enabled_override());
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());

  bool notified = false;
  auto subscription = controller->AddEnabledChangedCallback(
      base::BindRepeating([](bool* notified) { *notified = true; }, &notified));

  controller->SetVerticalTabsEnabledForWindow(true);

  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(notified);
}

TEST_F(VerticalTabControllerUnitTest,
       SharedSettingChangeRejoinsWindowWhenItMatches) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  auto controller = MakeController();
  controller->SetVerticalTabsEnabledForWindow(false);

  bool notified = false;
  auto subscription = controller->AddEnabledChangedCallback(
      base::BindRepeating([](bool* notified) { *notified = true; }, &notified));

  // The shared setting now matches this window, so the exception ends.
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, false);

  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(notified);

  // This window follows the switch again.
  notified = false;
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(notified);
}

TEST_F(VerticalTabControllerUnitTest, FollowProfilePrefNoOpWithoutOverride) {
  auto controller = MakeController();
  bool notified = false;
  auto subscription = controller->AddEnabledChangedCallback(
      base::BindRepeating([](bool* notified) { *notified = true; }, &notified));

  controller->FollowProfilePref();

  EXPECT_FALSE(notified);
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
}

TEST_F(VerticalTabControllerUnitTest, InheritedOverrideKeptWhenItDisagrees) {
  [[maybe_unused]] auto inherit = InheritEnabledOverrideForTesting(true);
  auto controller = MakeController();
  EXPECT_EQ(std::optional<bool>(true),
            controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(pref_service_.GetBoolean(brave_tabs::kVerticalTabsEnabled));
}

TEST_F(VerticalTabControllerUnitTest,
       InheritedOverrideMatchingPrefFollowsProfilePref) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  [[maybe_unused]] auto inherit = InheritEnabledOverrideForTesting(true);
  auto controller = MakeController();
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, InheritedFalseOverridesProfilePref) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  [[maybe_unused]] auto inherit = InheritEnabledOverrideForTesting(false);
  auto controller = MakeController();
  EXPECT_EQ(std::optional<bool>(false),
            controller->vertical_tabs_enabled_override());
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, EndedScopeDoesNotInherit) {
  {
    [[maybe_unused]] auto inherit = InheritEnabledOverrideForTesting(true);
  }
  auto controller = MakeController();
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
}

TEST_F(VerticalTabControllerUnitTest,
       SessionOverrideMatchingPrefFollowsProfilePref) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  [[maybe_unused]] auto inherit =
      VerticalTabController::InheritEnabledOverrideFromSession(
          BrowserWindowInterface::TYPE_NORMAL,
          {{kBraveVerticalTabsEnabledKey, "true"}});
  auto controller = MakeController();
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, SessionOverrideRestoredForNormalWindow) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  [[maybe_unused]] auto inherit =
      VerticalTabController::InheritEnabledOverrideFromSession(
          BrowserWindowInterface::TYPE_NORMAL,
          {{kBraveVerticalTabsEnabledKey, "false"}});
  auto controller = MakeController();
  EXPECT_EQ(std::optional<bool>(false),
            controller->vertical_tabs_enabled_override());
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(pref_service_.GetBoolean(brave_tabs::kVerticalTabsEnabled));
}

TEST_F(VerticalTabControllerUnitTest,
       SessionWithoutOverrideFollowsProfilePref) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  [[maybe_unused]] auto inherit =
      VerticalTabController::InheritEnabledOverrideFromSession(
          BrowserWindowInterface::TYPE_NORMAL, {});
  auto controller = MakeController();
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, EmptySessionOverrideFollowsProfilePref) {
  [[maybe_unused]] auto inherit =
      VerticalTabController::InheritEnabledOverrideFromSession(
          BrowserWindowInterface::TYPE_NORMAL,
          {{kBraveVerticalTabsEnabledKey, ""}});
  auto controller = MakeController();
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest,
       SessionOverrideRestoredWhenProfilePrefIsOff) {
  [[maybe_unused]] auto inherit =
      VerticalTabController::InheritEnabledOverrideFromSession(
          BrowserWindowInterface::TYPE_NORMAL,
          {{kBraveVerticalTabsEnabledKey, "true"}});
  auto controller = MakeController();
  EXPECT_EQ(std::optional<bool>(true),
            controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(pref_service_.GetBoolean(brave_tabs::kVerticalTabsEnabled));
}

TEST_F(VerticalTabControllerUnitTest,
       PopupSessionDoesNotClobberPendingOverride) {
  [[maybe_unused]] auto outer = InheritEnabledOverrideForTesting(true);
  [[maybe_unused]] auto inner =
      VerticalTabController::InheritEnabledOverrideFromSession(
          BrowserWindowInterface::TYPE_POPUP,
          {{kBraveVerticalTabsEnabledKey, "false"}});
  auto normal = MakeController();
  EXPECT_EQ(std::optional<bool>(true),
            normal->vertical_tabs_enabled_override());
  EXPECT_TRUE(normal->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, InheritedNulloptFollowsProfilePref) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  [[maybe_unused]] auto inherit =
      InheritEnabledOverrideForTesting(std::nullopt);
  auto controller = MakeController();
  EXPECT_EQ(std::nullopt, controller->vertical_tabs_enabled_override());
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, IsVerticalTabOnRightWhenSet) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsOnRight, true);
  auto controller = MakeController();
  EXPECT_TRUE(controller->IsVerticalTabOnRight());
}
