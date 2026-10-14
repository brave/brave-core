// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_geometry_util.h"
#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(CustomGestureRecognizerTest, ReturnsEmptyWithNoCustomGestures) {
  CustomGestureRecognizer custom_gesture_recognizer;

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeVStrokePoints()),
      testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, ReturnsEmptyForPathWithFewerThanTwoPoints) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab)}));

  EXPECT_THAT(custom_gesture_recognizer.ActionsForMatch({gfx::PointF()}),
              testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, MatchesIdenticalPath) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeVStrokePoints()),
      testing::ElementsAre(
          testing::Eq(MakeGestureAction(GestureCommandType::kNewTab))));
}

TEST(CustomGestureRecognizerTest, MatchesPathScaledAndTranslated) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab)}));

  std::vector<gfx::PointF> scaled_and_moved = {gfx::PointF(100.0, 150.0),
                                               gfx::PointF(140.0, 190.0),
                                               gfx::PointF(220.0, 50.0)};

  EXPECT_THAT(custom_gesture_recognizer.ActionsForMatch(scaled_and_moved),
              testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kNewTab))));
}

TEST(CustomGestureRecognizerTest, DoesNotMatchDissimilarShape) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab)}));

  // A straight horizontal line looks nothing like the V-shaped gesture.
  std::vector<gfx::PointF> straight_line = {gfx::PointF(),
                                            gfx::PointF(100.0, 0.0)};

  EXPECT_THAT(custom_gesture_recognizer.ActionsForMatch(straight_line),
              testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, RemoveStopsMatching) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab)}));

  custom_gesture_recognizer.Remove("id");

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeVStrokePoints()),
      testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, AddOverwritesExistingId) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab)}));
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kCloseTab)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeVStrokePoints()),
      testing::ElementsAre(
          testing::Eq(MakeGestureAction(GestureCommandType::kCloseTab))));
  EXPECT_THAT(custom_gesture_recognizer.custom_gestures(), testing::SizeIs(1));
}

TEST(CustomGestureRecognizerTest,
     ResampleAndNormalizeProducesRequestedPointCount) {
  EXPECT_THAT(ResampleAndNormalizePath(test::MakeVStrokePoints(), 10),
              testing::SizeIs(10));
}

TEST(CustomGestureRecognizerTest,
     MatchesClosedShapeStartedFromDifferentVertex) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeTriangleStrokePoints(/*start_vertex=*/0),
      {MakeGestureAction(GestureCommandType::kNewTab)}));

  // Same triangle, same direction, but traced starting from a different
  // vertex than the custom gesture was recorded with.
  EXPECT_THAT(custom_gesture_recognizer.ActionsForMatch(
                  test::MakeTriangleStrokePoints(/*start_vertex=*/1)),
              testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kNewTab))));
}

TEST(CustomGestureRecognizerTest,
     DoesNotMatchOpenShapeStartedFromDifferentPoint) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab)}));

  // The V shape is open, so unlike a closed shape, retracing it starting
  // from a different point produces a genuinely different path and should
  // not match.
  const std::vector<gfx::PointF>& v_shape = test::MakeVStrokePoints();
  std::vector<gfx::PointF> started_from_second_point = {
      v_shape.at(1), v_shape.at(2), v_shape.at(0)};
  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(started_from_second_point),
      testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, ActionsForMatchReturnsMatchedGestureUrl) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {test::MakeNavigateToUrlGestureAction("https://brave.com")}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeVStrokePoints()),
      testing::ElementsAre(testing::Eq(
          test::MakeNavigateToUrlGestureAction("https://brave.com"))));
}

TEST(CustomGestureRecognizerTest, ActionsForMatchReturnsAllChainedActions) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeVStrokePoints(),
      {MakeGestureAction(GestureCommandType::kNewTab),
       test::MakeNavigateToUrlGestureAction("https://brave.com")}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeVStrokePoints()),
      testing::ElementsAre(
          testing::Eq(MakeGestureAction(GestureCommandType::kNewTab)),
          testing::Eq(
              test::MakeNavigateToUrlGestureAction("https://brave.com"))));
}

}  // namespace gestures
