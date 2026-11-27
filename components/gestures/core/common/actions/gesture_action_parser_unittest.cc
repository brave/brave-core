// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_parser.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"
#include "brave/components/gestures/core/common/test/fake_gesture_commands.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GestureActionParserTest, ShouldReturnNoResultForEmptyList) {
  const base::ListValue list;

  EXPECT_FALSE(MaybeParseGestureActions(list));
}

TEST(GestureActionParserTest, ShouldRejectEntryWithoutCommand) {
  const base::ListValue list = base::ListValue().Append(base::DictValue());

  EXPECT_FALSE(MaybeParseGestureActions(list));
}

TEST(GestureActionParserTest, ShouldParseSwitchToTabCommand) {
  const base::ListValue list =
      base::ListValue().Append(base::DictValue()
                                   .Set("command", kSwitchToTab)
                                   .Set("text_param", "*gmail*"));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(testing::FieldsAre(
                  kSwitchToTab, testing::VariantWith<TextParamInfo>(
                                    testing::FieldsAre("*gmail*"))))));
}

TEST(GestureActionParserTest, ShouldRejectEntryWithInvalidCommand) {
  const base::ListValue list =
      base::ListValue().Append(base::DictValue().Set("command", 9999));

  EXPECT_FALSE(MaybeParseGestureActions(list));
}

TEST(GestureActionParserTest, ShouldParseCommandWithoutParameters) {
  const base::ListValue list = base::ListValue().Append(
      base::DictValue().Set("command", test::kFakeGestureCommand));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(
                  MakeGestureAction(test::kFakeGestureCommand))));
}

TEST(GestureActionParserTest, ShouldParseNavigateToUrlCommand) {
  const base::ListValue list =
      base::ListValue().Append(base::DictValue()
                                   .Set("command", kNavigateToUrl)
                                   .Set("text_param", "https://brave.com"));

  EXPECT_THAT(
      MaybeParseGestureActions(list),
      testing::Optional(testing::ElementsAre(testing::FieldsAre(
          kNavigateToUrl, testing::VariantWith<TextParamInfo>(
                              testing::FieldsAre("https://brave.com"))))));
}

TEST(GestureActionParserTest, ShouldParseOpenSettingsCommand) {
  const base::ListValue list = base::ListValue().Append(
      base::DictValue()
          .Set("command", test::kIdcOptionsCommand)
          .Set("value_param",
               static_cast<int>(OpenSettingsPageType::kPrivacy)));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(testing::FieldsAre(
                  test::kIdcOptionsCommand,
                  testing::VariantWith<IntParamInfo>(testing::FieldsAre(
                      std::to_underlying(OpenSettingsPageType::kPrivacy)))))));
}

TEST(GestureActionParserTest, ShouldSkipInvalidEntryAndKeepValidOnes) {
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue())
          .Append(base::DictValue().Set("command",
                                        test::kAnotherFakeGestureCommand));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(
                  MakeGestureAction(test::kAnotherFakeGestureCommand))));
}

TEST(GestureActionParserTest, ShouldParseAllChainedActions) {
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue().Set("command",
                                        test::kAnotherFakeGestureCommand))
          .Append(base::DictValue().Set("command",
                                        test::kYetAnotherFakeGestureCommand));

  EXPECT_THAT(MaybeParseGestureActions(list),
              testing::Optional(testing::ElementsAre(
                  MakeGestureAction(test::kAnotherFakeGestureCommand),
                  MakeGestureAction(test::kYetAnotherFakeGestureCommand))));
}

}  // namespace gestures
