// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_parser.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/test/fake_gesture_commands.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

TEST(CustomGestureParserTest, ShouldRejectEntryWithoutId) {
  const base::DictValue dict =
      base::DictValue()
          .Set("points", base::ListValue())
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", test::kFakeGestureCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGestureParserTest, ShouldRejectEntryWithoutPoints) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "1792208323001")
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", test::kFakeGestureCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGestureParserTest, ShouldRejectEntryWithNoValidActions) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "1792208323001")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions", base::ListValue());

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGestureParserTest, ShouldRejectEntryWithFewerThanTwoPoints) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "1792208323001")
          .Set("points", base::ListValue().Append(
                             base::DictValue().Set("x", 0.0).Set("y", 0.0)))
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", test::kFakeGestureCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGestureParserTest,
     ShouldSkipInvalidPointsWhenOtherValidPointsRemain) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "1792208323001")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   // Missing "y"; skipped rather than failing the whole
                   // gesture.
                   .Append(base::DictValue().Set("x", 5.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", test::kFakeGestureCommand)));

  EXPECT_THAT(
      MaybeParseCustomGesture(dict),
      testing::Optional(testing::FieldsAre(
          /*id=*/"1792208323001",
          /*gesture_path=*/
          testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 1.0)),
          /*actions=*/
          testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)))));
}

TEST(CustomGestureParserTest, ShouldParseValidEntry) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "1792208323001")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", test::kFakeGestureCommand)));

  EXPECT_THAT(
      MaybeParseCustomGesture(dict),
      testing::Optional(testing::FieldsAre(
          /*id=*/"1792208323001",
          /*gesture_path=*/
          testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 1.0)),
          /*actions=*/
          testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)))));
}

TEST(CustomGestureParserTest, ShouldParseAllChainedActions) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "1792208323001")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions",
               base::ListValue()
                   .Append(base::DictValue().Set("command",
                                                 test::kFakeGestureCommand))
                   .Append(base::DictValue().Set(
                       "command", test::kAnotherFakeGestureCommand)));

  EXPECT_THAT(
      MaybeParseCustomGesture(dict),
      testing::Optional(testing::FieldsAre(
          /*id=*/"1792208323001",
          /*gesture_path=*/
          testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 1.0)),
          /*actions=*/
          testing::ElementsAre(
              MakeGestureAction(test::kFakeGestureCommand),
              MakeGestureAction(test::kAnotherFakeGestureCommand)))));
}

}  // namespace gestures
