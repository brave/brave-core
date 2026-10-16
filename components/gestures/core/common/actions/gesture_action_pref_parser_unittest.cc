// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_pref_parser.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// Stands in for a real browser `IDC_*` command id in tests. This component
// must not depend on `IDC_*` headers, and the exact value doesn't matter
// here, only that it's recognized as a real command (>= 30000).
constexpr int kSomeRealCommand = 33000;
constexpr int kAnotherRealCommand = 33001;
constexpr int kYetAnotherRealCommand = 34014;

// Stands in for the real `IDC_OPTIONS` id (40015), same reasoning as
// `kSomeRealCommand`.
constexpr int kOpenSettingsCommand = 40015;

}  // namespace

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsReturnsNulloptForEmptyList) {
  base::ListValue list;

  EXPECT_FALSE(MaybeParseGestureActions(list));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsReturnsNulloptWhenEntryMissingCommand) {
  base::ListValue list;
  list.Append(base::DictValue());

  EXPECT_FALSE(MaybeParseGestureActions(list));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesSwitchToTabCommand) {
  base::ListValue list;
  list.Append(base::DictValue()
                  .Set("command", kSwitchToTab)
                  .Set("text_param", "*gmail*"));

  const std::optional<std::vector<GestureActionInfo>> actions =
      MaybeParseGestureActions(list);
  ASSERT_THAT(actions, testing::Optional(testing::SizeIs(1)));

  const GestureActionInfo& action = actions->at(0);
  EXPECT_EQ(kSwitchToTab, action.command);
  const auto* switch_to_tab_params =
      std::get_if<TextParamInfo>(&action.command_params);
  ASSERT_TRUE(switch_to_tab_params);
  EXPECT_EQ("*gmail*", switch_to_tab_params->text_param);
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsReturnsNulloptWhenEntryHasInvalidCommand) {
  base::ListValue list;
  list.Append(base::DictValue().Set("command", 9999));

  EXPECT_FALSE(MaybeParseGestureActions(list));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesACommandWithNoParams) {
  base::ListValue list;
  list.Append(base::DictValue().Set("command", kSomeRealCommand));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(
                  testing::Eq(MakeGestureAction(kSomeRealCommand)))));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesANavigateToUrlCommand) {
  base::ListValue list;
  list.Append(base::DictValue()
                  .Set("command", kNavigateToUrl)
                  .Set("text_param", "https://brave.com"));

  const std::optional<std::vector<GestureActionInfo>> actions =
      MaybeParseGestureActions(list);
  ASSERT_THAT(actions, testing::Optional(testing::SizeIs(1)));

  const GestureActionInfo& action = actions->at(0);
  EXPECT_EQ(kNavigateToUrl, action.command);
  const auto* navigate_to_url_params =
      std::get_if<TextParamInfo>(&action.command_params);
  ASSERT_TRUE(navigate_to_url_params);
  EXPECT_EQ("https://brave.com", navigate_to_url_params->text_param);
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesAnOpenSettingsCommand) {
  base::ListValue list;
  list.Append(
      base::DictValue()
          .Set("command", kOpenSettingsCommand)
          .Set("value_param", static_cast<int>(OpenSettingsPage::kPrivacy)));

  const std::optional<std::vector<GestureActionInfo>> actions =
      MaybeParseGestureActions(list);
  ASSERT_THAT(actions, testing::Optional(testing::SizeIs(1)));

  const GestureActionInfo& action = actions->at(0);
  EXPECT_EQ(kOpenSettingsCommand, action.command);
  const auto* int_param_info =
      std::get_if<IntParamInfo>(&action.command_params);
  ASSERT_TRUE(int_param_info);
  EXPECT_EQ(std::to_underlying(OpenSettingsPage::kPrivacy),
            int_param_info->value_param);
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsSkipsInvalidEntryKeepingOthers) {
  base::ListValue list;
  list.Append(base::DictValue());
  list.Append(base::DictValue().Set("command", kAnotherRealCommand));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(
                  testing::Eq(MakeGestureAction(kAnotherRealCommand)))));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesEveryStoredChainedAction) {
  base::ListValue list;
  list.Append(base::DictValue().Set("command", kAnotherRealCommand));
  list.Append(base::DictValue().Set("command", kYetAnotherRealCommand));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(
                  testing::Eq(MakeGestureAction(kAnotherRealCommand)),
                  testing::Eq(MakeGestureAction(kYetAnotherRealCommand)))));
}

}  // namespace gestures
