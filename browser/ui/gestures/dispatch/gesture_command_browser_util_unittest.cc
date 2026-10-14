// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/gesture_command_browser_util.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/app/command_utils.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

TEST(GestureCommandBrowserUtilTest, ShouldPreserveBrowserCommandId) {
  EXPECT_EQ(IDC_BACK, MaybeToGestureCommandId(IDC_BACK));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldReturnNoBrowserCommandIdForUnassignedCommand) {
  EXPECT_FALSE(MaybeToGestureCommandId(kUnassigned));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldReturnNoBrowserCommandIdForNavigateToUrlCommand) {
  EXPECT_FALSE(MaybeToGestureCommandId(kNavigateToUrl));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldReturnNoBrowserCommandIdForSwitchToTabCommand) {
  EXPECT_FALSE(MaybeToGestureCommandId(kSwitchToTab));
}

TEST(GestureCommandBrowserUtilTest, ShouldUseLocalizedNameForSpecialCommand) {
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_UNASSIGNED),
            ToCommandDisplayName(kUnassigned));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldUseDisplayNameAssociatedWithBrowserCommandId) {
  EXPECT_EQ(base::UTF8ToUTF16(commands::GetCommandName(IDC_BACK)),
            ToCommandDisplayName(IDC_BACK));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldUseDifferentNamesForDifferentBrowserCommands) {
  EXPECT_NE(ToCommandDisplayName(IDC_BACK), ToCommandDisplayName(IDC_FORWARD));
}

TEST(GestureCommandBrowserUtilTest, ShouldReturnEmptyDisplayNameForNoActions) {
  EXPECT_THAT(ToActionsDisplayName({}), testing::IsEmpty());
}

TEST(GestureCommandBrowserUtilTest, ShouldPreserveDisplayNameForSingleAction) {
  EXPECT_EQ(ToCommandDisplayName(IDC_BACK),
            ToActionsDisplayName({MakeGestureAction(IDC_BACK)}));
}

TEST(GestureCommandBrowserUtilTest, ShouldJoinChainedActionNamesWithChevron) {
  EXPECT_EQ(ToCommandDisplayName(IDC_BACK) + kActionsDisplayNameSeparator +
                ToCommandDisplayName(IDC_FORWARD),
            ToActionsDisplayName(
                {MakeGestureAction(IDC_BACK), MakeGestureAction(IDC_FORWARD)}));
}

TEST(GestureCommandBrowserUtilTest, ShouldIncludeUrlForNavigateToUrlAction) {
  EXPECT_EQ(ToCommandDisplayName(kNavigateToUrl) + u" (https://brave.com/)",
            ToActionsDisplayName(
                {test::MakeNavigateToUrlGestureAction("https://brave.com/")}));
}

TEST(GestureCommandBrowserUtilTest, ShouldIncludePatternForSwitchToTabAction) {
  EXPECT_EQ(
      ToCommandDisplayName(kSwitchToTab) + u" (*title1*)",
      ToActionsDisplayName({test::MakeSwitchToTabGestureAction("*title1*")}));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldIncludeSubPageForOpenSettingsPageAction) {
  EXPECT_EQ(ToCommandDisplayName(IDC_OPTIONS) + u" (leo-ai)",
            ToActionsDisplayName({test::MakeOpenSettingsPageGestureAction(
                OpenSettingsPageType::kLeoAi)}));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldIncludeSubPageForOpenSettingsPageActionWithinAChain) {
  EXPECT_EQ(ToCommandDisplayName(IDC_BACK) + kActionsDisplayNameSeparator +
                ToCommandDisplayName(IDC_OPTIONS) + u" (leo-ai)",
            ToActionsDisplayName({MakeGestureAction(IDC_BACK),
                                  test::MakeOpenSettingsPageGestureAction(
                                      OpenSettingsPageType::kLeoAi)}));
}

TEST(GestureCommandBrowserUtilTest,
     ShouldOmitSubPageForBasicOpenSettingsPageAction) {
  EXPECT_EQ(ToCommandDisplayName(IDC_OPTIONS),
            ToActionsDisplayName({test::MakeOpenSettingsPageGestureAction(
                OpenSettingsPageType::kBasic)}));
}

}  // namespace gestures
