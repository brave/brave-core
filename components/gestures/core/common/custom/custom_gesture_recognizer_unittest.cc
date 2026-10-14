// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"
#include "brave/components/gestures/core/common/gesture_geometry_util.h"
#include "brave/components/gestures/core/common/test/fake_gesture_commands.h"
#include "brave/components/gestures/core/common/test/gesture_path_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(CustomGestureRecognizerTest, ShouldReturnNoMatchWhenNoGesturesExist) {
  CustomGestureRecognizer custom_gesture_recognizer;

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeLetterVGesturePath()),
      testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest,
     ShouldReturnNoMatchForPathWithFewerThanTwoPoints) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(custom_gesture_recognizer.ActionsForMatch({gfx::PointF()}),
              testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, ShouldMatchIdenticalPaths) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeLetterVGesturePath()),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldMatchPathsRegardlessOfSizeAndPosition) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath original_path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", original_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // The same shape, scaled 2x and translated, rather than a hand-picked
  // substitute path, so this stays a true scale/position invariance check.
  GesturePath scaled_and_moved;
  for (const gfx::PointF& point : original_path) {
    scaled_and_moved.emplace_back(point.x() * 2 + 100, point.y() * 2 + 50);
  }

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(scaled_and_moved),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest,
     ShouldMatchSameShapeDespiteUnevenPointDensity) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath original_path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", original_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // The same shape, but traced with a mouse that paused right at the start
  // (many duplicate points) before continuing normally — arc-length
  // resampling should normalize away the difference rather than letting it
  // skew the match.
  GesturePath unevenly_sampled_path(10, original_path.front());
  unevenly_sampled_path.insert(unevenly_sampled_path.end(),
                               original_path.begin(), original_path.end());

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(unevenly_sampled_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldNotMatchDissimilarShapes) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // A straight horizontal line looks nothing like the V-shaped gesture.
  GesturePath straight_line = {gfx::PointF(), gfx::PointF(100.0, 0.0)};

  EXPECT_THAT(custom_gesture_recognizer.ActionsForMatch(straight_line),
              testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, ShouldNotMatchAfterRemoval) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  custom_gesture_recognizer.Remove("id");

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeLetterVGesturePath()),
      testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, ShouldReplaceExistingEntryWithSameId) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kAnotherFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeLetterVGesturePath()),
      testing::ElementsAre(
          MakeGestureAction(test::kAnotherFakeGestureCommand)));
  EXPECT_THAT(custom_gesture_recognizer.custom_gestures(), testing::SizeIs(1));
}

TEST(CustomGestureRecognizerTest,
     ShouldProduceRequestedPointCountAfterResampling) {
  EXPECT_THAT(ResampleAndNormalizePath(test::MakeLetterVGesturePath(), 10),
              testing::SizeIs(10));
}

TEST(CustomGestureRecognizerTest,
     ShouldMatchClosedShapeFromDifferentStartingPoint) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath triangle_path = test::MakeTriangleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", triangle_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // Same triangle, same direction, but retraced starting partway along the
  // path rather than from its original first point, since it's a closed
  // shape.
  const auto rotation_offset = base::checked_cast<GesturePath::difference_type>(
      triangle_path.size() / 2);
  GesturePath retraced_from_different_point(
      triangle_path.begin() + rotation_offset, triangle_path.end());
  retraced_from_different_point.insert(
      retraced_from_different_point.end(), triangle_path.begin(),
      triangle_path.begin() + rotation_offset + 1);
  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(retraced_from_different_point),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest,
     ShouldNotMatchOpenShapeFromDifferentStartingPoint) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath v_gesture_path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", v_gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // The V shape is open, so unlike a closed shape, retracing it starting
  // from a different point produces a genuinely different path and should
  // not match.
  GesturePath started_from_second_point(v_gesture_path.begin() + 1,
                                        v_gesture_path.end());
  started_from_second_point.push_back(v_gesture_path.front());
  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(started_from_second_point),
      testing::IsEmpty());
}

TEST(CustomGestureRecognizerTest, ShouldReturnUrlForMatch) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {test::MakeNavigateToUrlGestureAction("https://brave.com")}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeLetterVGesturePath()),
      testing::ElementsAre(
          test::MakeNavigateToUrlGestureAction("https://brave.com")));
}

TEST(CustomGestureRecognizerTest, ShouldReturnAllChainedActionsForMatch) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand),
       test::MakeNavigateToUrlGestureAction("https://brave.com")}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(test::MakeLetterVGesturePath()),
      testing::ElementsAre(
          MakeGestureAction(test::kFakeGestureCommand),
          test::MakeNavigateToUrlGestureAction("https://brave.com")));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeTriangle) {
  // A real gesture (triangle shape, mapped to "New tab in group"), pinned here
  // so future changes to the matching pipeline can't silently stop recognizing
  // it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeTriangleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792208323001", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeZigzag) {
  // A real gesture (zigzag shape, mapped to "Navigate to URL" brave.com),
  // pinned here so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeZigzagGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792208750560", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeLetterV) {
  // A real gesture (letter V shape, unassigned), pinned here so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212339556", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeConjunction) {
  // A real gesture (conjunction shape, unassigned), pinned here so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeConjunctionGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212345544", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeCircle) {
  // A real gesture (circle shape, unassigned), pinned here so future changes
  // to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeCircleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212354892", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeSpiral) {
  // A real gesture (spiral/@ shape, mapped to "Add new tab to group"), pinned
  // here so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeSpiralGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212359358", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeHandwriting) {
  // A real gesture (handwriting shape, mapped to "Open Settings Page"
  // /leo-ai), pinned here so future changes to the matching pipeline can't
  // silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeHandwritingGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212468887", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeHook) {
  // A real gesture (hook shape, mapped to "About Brave VPN"), pinned here so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeHookGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792217771449", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeRocket) {
  // A real gesture (rocket shape, mapped to "About Brave"), pinned here so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeRocketGesturePath();
  custom_gesture_recognizer.Add(
      test::MakeCustomGesture(/*id=*/"1792218580198", path,
                              {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeDiamond) {
  // A real gesture (diamond shape, mapped to "New tab to the right"), pinned
  // here so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeDiamondGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240640644", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeOval) {
  // A real gesture (oval shape, mapped to unassigned), pinned here so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeOvalGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240667421", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeLeftOpenFrame) {
  // A real gesture (left-open frame shape, mapped to unassigned), pinned
  // here so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeLeftOpenFrameGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240836620", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeRightOpenFrame) {
  // A real gesture (right-open frame shape, mapped to "Copy Clean Link"),
  // pinned here so future changes to the matching pipeline can't silently
  // stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeRightOpenFrameGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240839975", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeSquareArch) {
  // A real gesture (square arch shape, mapped to unassigned), pinned here so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeSquareArchGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240843266", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeIntersection) {
  // A real gesture (intersection shape, mapped to unassigned), pinned here
  // so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeIntersectionGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240845970", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeLetterC) {
  // A real gesture (letter C shape, mapped to "Copy"), pinned here so
  // future changes to the matching pipeline can't silently stop recognizing
  // it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeLetterCGesturePath();
  custom_gesture_recognizer.Add(
      test::MakeCustomGesture(/*id=*/"1792240848916", path,
                              {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeLetterU) {
  // A real gesture (letter U shape, mapped to unassigned), pinned here so
  // future changes to the matching pipeline can't silently stop recognizing
  // it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeLetterUGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240851475", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeReversedC) {
  // A real gesture (reversed C shape, mapped to unassigned), pinned here so
  // future changes to the matching pipeline can't silently stop recognizing
  // it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeReversedCGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240854497", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeAngledCorner) {
  // A real gesture (angled corner shape, mapped to unassigned), pinned here so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeAngledCornerGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240860585", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeSquiggle) {
  // A real gesture (squiggle shape, mapped to unassigned), pinned
  // here so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeSquiggleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240876584", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeDoublePeak) {
  // A real gesture (double peak shape, mapped to "Bookmark bar edit"),
  // pinned here so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeDoublePeakGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240882328", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST(CustomGestureRecognizerTest, ShouldRecognizeCoil) {
  // A real gesture (coil shape, mapped to "Block elements"),
  // pinned here so future changes to the matching pipeline can't silently stop
  // recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath path = test::MakeCoilGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792241147635", path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      custom_gesture_recognizer.ActionsForMatch(path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

}  // namespace gestures
