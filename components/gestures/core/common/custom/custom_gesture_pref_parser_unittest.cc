// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_pref_parser.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

namespace {

// Stand in for real browser `IDC_*` command ids in tests. This component
// must not depend on `IDC_*` headers, and the exact values don't matter
// here, only that they're recognized as real commands (>= 30000).
constexpr int kSomeRealCommand = 33000;
constexpr int kAnotherRealCommand = 33001;

}  // namespace

TEST(CustomGesturePrefParserTest, ShouldRejectEntryWithoutId) {
  const base::DictValue dict =
      base::DictValue()
          .Set("points", base::ListValue())
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", kSomeRealCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGesturePrefParserTest, ShouldRejectEntryWithoutPoints) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "id")
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", kSomeRealCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGesturePrefParserTest, ShouldRejectEntryWithoutValidActions) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "id")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions", base::ListValue());

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGesturePrefParserTest, ShouldRejectEntryWithFewerThanTwoPoints) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "id")
          .Set("points", base::ListValue().Append(
                             base::DictValue().Set("x", 0.0).Set("y", 0.0)))
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", kSomeRealCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(dict));
}

TEST(CustomGesturePrefParserTest,
     ShouldSkipInvalidPointsWhenOtherValidPointsRemain) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "id")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   // Missing "y"; skipped rather than failing the whole
                   // gesture.
                   .Append(base::DictValue().Set("x", 5.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", kSomeRealCommand)));

  EXPECT_THAT(
      MaybeParseCustomGesture(dict),
      testing::Optional(testing::FieldsAre(
          "id",
          /*path=*/
          testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 1.0)),
          /*actions=*/
          testing::ElementsAre(MakeGestureAction(kSomeRealCommand)))));
}

TEST(CustomGesturePrefParserTest, ShouldParseValidEntry) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "id")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions", base::ListValue().Append(base::DictValue().Set(
                              "command", kSomeRealCommand)));

  EXPECT_THAT(
      MaybeParseCustomGesture(dict),
      testing::Optional(testing::FieldsAre(
          "id",
          /*path=*/
          testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 1.0)),
          /*actions=*/
          testing::ElementsAre(MakeGestureAction(kSomeRealCommand)))));
}

TEST(CustomGesturePrefParserTest, ShouldParseAllChainedActions) {
  const base::DictValue dict =
      base::DictValue()
          .Set("id", "id")
          .Set("points",
               base::ListValue()
                   .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                   .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
          .Set("actions",
               base::ListValue()
                   .Append(base::DictValue().Set("command", kSomeRealCommand))
                   .Append(
                       base::DictValue().Set("command", kAnotherRealCommand)));

  EXPECT_THAT(
      MaybeParseCustomGesture(dict),
      testing::Optional(testing::FieldsAre(
          /*id=*/testing::_, /*path=*/testing::_,
          /*actions=*/
          testing::ElementsAre(MakeGestureAction(kSomeRealCommand),
                               MakeGestureAction(kAnotherRealCommand)))));
}

}  // namespace gestures
