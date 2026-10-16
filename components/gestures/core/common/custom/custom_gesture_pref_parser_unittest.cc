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

// Stands in for a real browser `IDC_*` command id in tests. This component
// must not depend on `IDC_*` headers, and the exact value doesn't matter
// here, only that it's recognized as a real command (>= 30000).
constexpr int kSomeRealCommand = 33000;

}  // namespace

TEST(CustomGesturePrefParserTest,
     MaybeParseCustomGestureReturnsNulloptMissingId) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("points", base::ListValue())
      .Set("actions", base::ListValue().Append(
                          base::DictValue().Set("command", kSomeRealCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(CustomGesturePrefParserTest,
     MaybeParseCustomGestureReturnsNulloptMissingPoints) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id")
      .Set("actions", base::ListValue().Append(
                          base::DictValue().Set("command", kSomeRealCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(CustomGesturePrefParserTest,
     MaybeParseCustomGestureReturnsNulloptWithNoValidActions) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id")
      .Set("points", base::ListValue()
                         .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                         .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
      .Set("actions", base::ListValue());

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(CustomGesturePrefParserTest,
     MaybeParseCustomGestureReturnsNulloptWithFewerThanTwoPoints) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id")
      .Set("points", base::ListValue().Append(
                         base::DictValue().Set("x", 0.0).Set("y", 0.0)))
      .Set("actions", base::ListValue().Append(
                          base::DictValue().Set("command", kSomeRealCommand)));

  EXPECT_FALSE(MaybeParseCustomGesture(custom_gesture_dict));
}

TEST(CustomGesturePrefParserTest, MaybeParseCustomGestureParsesAValidEntry) {
  base::DictValue custom_gesture_dict;
  custom_gesture_dict.Set("id", "id")
      .Set("points", base::ListValue()
                         .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                         .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
      .Set("actions", base::ListValue().Append(
                          base::DictValue().Set("command", kSomeRealCommand)));

  const std::optional<CustomGestureInfo> custom_gesture =
      MaybeParseCustomGesture(custom_gesture_dict);

  ASSERT_TRUE(custom_gesture);
  EXPECT_EQ("id", custom_gesture->id);
  EXPECT_THAT(
      custom_gesture->stroke_points,
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 1.0)));
  EXPECT_THAT(
      custom_gesture->actions,
      testing::ElementsAre(testing::Eq(MakeGestureAction(kSomeRealCommand))));
}

}  // namespace gestures
