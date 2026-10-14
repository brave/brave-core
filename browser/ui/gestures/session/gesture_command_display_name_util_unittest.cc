// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_command_display_name_util.h"

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
#include "url/gurl.h"

namespace gestures {

TEST(GestureCommandDisplayNameUtilTest,
     ShouldUseLocalizedNameForSpecialCommand) {
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_UNASSIGNED),
            ToGestureCommandDisplayName(kUnassigned));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldUseDisplayNameAssociatedWithBrowserCommandId) {
  EXPECT_EQ(base::UTF8ToUTF16(commands::GetCommandName(IDC_BACK)),
            ToGestureCommandDisplayName(IDC_BACK));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldUseDifferentNamesForDifferentBrowserCommands) {
  EXPECT_NE(ToGestureCommandDisplayName(IDC_BACK),
            ToGestureCommandDisplayName(IDC_FORWARD));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldReturnEmptyDisplayNameForNoActions) {
  EXPECT_THAT(ToGestureActionsDisplayName({}), testing::IsEmpty());
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldPreserveDisplayNameForSingleAction) {
  EXPECT_EQ(ToGestureCommandDisplayName(IDC_BACK),
            ToGestureActionsDisplayName({MakeGestureAction(IDC_BACK)}));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldJoinChainedActionNamesWithChevron) {
  std::u16string expected_display_name = ToGestureCommandDisplayName(IDC_BACK);
  expected_display_name += kActionsDisplayNameSeparator;
  expected_display_name += ToGestureCommandDisplayName(IDC_FORWARD);

  EXPECT_EQ(expected_display_name,
            ToGestureActionsDisplayName(
                {MakeGestureAction(IDC_BACK), MakeGestureAction(IDC_FORWARD)}));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldIncludeUrlForNavigateToUrlAction) {
  EXPECT_EQ(
      ToGestureCommandDisplayName(kNavigateToUrl) + u" (https://brave.com/)",
      ToGestureActionsDisplayName(
          {test::MakeNavigateToUrlGestureAction(GURL("https://brave.com/"))}));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldIncludePatternForSwitchToTabAction) {
  EXPECT_EQ(ToGestureCommandDisplayName(kSwitchToTab) + u" (*title1*)",
            ToGestureActionsDisplayName(
                {test::MakeSwitchToTabGestureAction("*title1*")}));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldIncludeSubPageForOpenSettingsPageAction) {
  EXPECT_EQ(
      ToGestureCommandDisplayName(IDC_OPTIONS) + u" (leo-ai)",
      ToGestureActionsDisplayName({test::MakeOpenSettingsPageGestureAction(
          OpenSettingsPageType::kLeoAi)}));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldIncludeSubPageForOpenSettingsPageActionWithinAChain) {
  std::u16string expected_display_name = ToGestureCommandDisplayName(IDC_BACK);
  expected_display_name += kActionsDisplayNameSeparator;
  expected_display_name +=
      ToGestureCommandDisplayName(IDC_OPTIONS) + u" (leo-ai)";

  EXPECT_EQ(expected_display_name, ToGestureActionsDisplayName(
                                       {MakeGestureAction(IDC_BACK),
                                        test::MakeOpenSettingsPageGestureAction(
                                            OpenSettingsPageType::kLeoAi)}));
}

TEST(GestureCommandDisplayNameUtilTest,
     ShouldOmitSubPageForBasicOpenSettingsPageAction) {
  EXPECT_EQ(
      ToGestureCommandDisplayName(IDC_OPTIONS),
      ToGestureActionsDisplayName({test::MakeOpenSettingsPageGestureAction(
          OpenSettingsPageType::kBasic)}));
}

}  // namespace gestures
