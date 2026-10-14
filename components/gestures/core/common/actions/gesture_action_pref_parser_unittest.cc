// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_pref_parser.h"

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsReturnsNulloptForEmptyList) {
  base::ListValue actions_list;

  EXPECT_FALSE(MaybeParseGestureActions(actions_list));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsReturnsNulloptWhenEntryMissingCommand) {
  base::ListValue actions_list;
  actions_list.Append(base::DictValue());

  EXPECT_FALSE(MaybeParseGestureActions(actions_list));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesSwitchToTabCommand) {
  base::ListValue actions_list;
  actions_list.Append(
      base::DictValue()
          .Set("command", static_cast<int>(GestureCommandType::kSwitchToTab))
          .Set("switch_to_tab_pattern", "*gmail*"));

  const std::optional<std::vector<GestureActionInfo>> actions =
      MaybeParseGestureActions(actions_list);

  ASSERT_TRUE(actions);
  ASSERT_THAT(*actions, testing::SizeIs(1));
  EXPECT_EQ(GestureCommandType::kSwitchToTab, actions->at(0).command);
  const auto* switch_to_tab_params =
      std::get_if<SwitchToTabParamsInfo>(&actions->at(0).command_params);
  ASSERT_TRUE(switch_to_tab_params);
  EXPECT_EQ("*gmail*", switch_to_tab_params->pattern);
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsReturnsNulloptWhenEntryHasInvalidCommand) {
  base::ListValue actions_list;
  actions_list.Append(base::DictValue().Set("command", 9999));

  EXPECT_FALSE(MaybeParseGestureActions(actions_list));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesACommandWithNoParams) {
  base::ListValue actions_list;
  actions_list.Append(base::DictValue().Set(
      "command", static_cast<int>(GestureCommandType::kBack)));

  EXPECT_THAT(MaybeParseGestureActions(actions_list),
              testing::Optional(testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kBack)))));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesANavigateToUrlCommand) {
  base::ListValue actions_list;
  actions_list.Append(
      base::DictValue()
          .Set("command", static_cast<int>(GestureCommandType::kNavigateToUrl))
          .Set("navigate_to_url", "https://brave.com"));

  const std::optional<std::vector<GestureActionInfo>> actions =
      MaybeParseGestureActions(actions_list);

  ASSERT_TRUE(actions);
  ASSERT_THAT(*actions, testing::SizeIs(1));
  EXPECT_EQ(GestureCommandType::kNavigateToUrl, actions->at(0).command);
  const auto* navigate_to_url_params =
      std::get_if<NavigateToUrlParamsInfo>(&actions->at(0).command_params);
  ASSERT_TRUE(navigate_to_url_params);
  EXPECT_EQ("https://brave.com", navigate_to_url_params->url_spec);
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsSkipsInvalidEntryKeepingOthers) {
  base::ListValue actions_list;
  actions_list.Append(base::DictValue());
  actions_list.Append(base::DictValue().Set(
      "command", static_cast<int>(GestureCommandType::kForward)));

  EXPECT_THAT(MaybeParseGestureActions(actions_list),
              testing::Optional(testing::ElementsAre(testing::Eq(
                  MakeGestureAction(GestureCommandType::kForward)))));
}

TEST(GestureActionPrefParserTest,
     MaybeParseGestureActionsParsesEveryStoredChainedAction) {
  base::ListValue actions_list;
  actions_list.Append(base::DictValue().Set(
      "command", static_cast<int>(GestureCommandType::kForward)));
  actions_list.Append(base::DictValue().Set(
      "command", static_cast<int>(GestureCommandType::kNewTab)));

  EXPECT_THAT(
      MaybeParseGestureActions(actions_list),
      testing::Optional(testing::ElementsAre(
          testing::Eq(MakeGestureAction(GestureCommandType::kForward)),
          testing::Eq(MakeGestureAction(GestureCommandType::kNewTab)))));
}

TEST(GestureActionPrefParserTest,
     MaybeParseCustomGestureReturnsNulloptMissingId) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("points", base::ListValue());
  custom_gesture_dict.Set(
      "actions", base::ListValue().Append(base::DictValue().Set(
                     "command", static_cast<int>(GestureCommandType::kBack))));

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(GestureActionPrefParserTest,
     MaybeParseCustomGestureReturnsNulloptMissingPoints) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id");
  custom_gesture_dict.Set(
      "actions", base::ListValue().Append(base::DictValue().Set(
                     "command", static_cast<int>(GestureCommandType::kBack))));

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(GestureActionPrefParserTest,
     MaybeParseCustomGestureReturnsNulloptWithNoValidActions) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id");
  custom_gesture_dict.Set(
      "points", base::ListValue()
                    .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                    .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)));
  custom_gesture_dict.Set("actions", base::ListValue());

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(GestureActionPrefParserTest,
     MaybeParseCustomGestureReturnsNulloptWithFewerThanTwoPoints) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id");
  custom_gesture_dict.Set(
      "points",
      base::ListValue().Append(base::DictValue().Set("x", 0.0).Set("y", 0.0)));
  custom_gesture_dict.Set(
      "actions", base::ListValue().Append(base::DictValue().Set(
                     "command", static_cast<int>(GestureCommandType::kBack))));

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(GestureActionPrefParserTest, MaybeParseCustomGestureParsesAValidEntry) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id");
  custom_gesture_dict.Set(
      "points", base::ListValue()
                    .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                    .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)));
  custom_gesture_dict.Set(
      "actions", base::ListValue().Append(base::DictValue().Set(
                     "command", static_cast<int>(GestureCommandType::kBack))));

  const std::optional<CustomGestureInfo> custom_gesture =
      MaybeParseCustomGesture(custom_gesture_dict);

  ASSERT_TRUE(custom_gesture);
  EXPECT_EQ("id", custom_gesture->id);
  EXPECT_THAT(
      custom_gesture->stroke_points,
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 1.0)));
  EXPECT_THAT(custom_gesture->actions,
              testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kBack))));
}

}  // namespace gestures
