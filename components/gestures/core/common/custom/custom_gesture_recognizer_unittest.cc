// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"

#include "base/numerics/safe_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/test/fake_gesture_commands.h"
#include "brave/components/gestures/core/common/test/gesture_path_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace gestures {

class CustomGestureRecognizerTest : public testing::Test {
 public:
  CustomGestureRecognizerTest() {
    // Pinned since `MaybeMatchGesture` reads these; a default change
    // shouldn't silently alter what these tests cover.
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"CustomGestureResamplePointCount", "128"},
         {"CustomGestureInlierPointThreshold", "0.1"},
         {"CustomGestureMaxMatchDistance", "0.14"},
         {"CustomGestureMinHitScore", "0.08"}});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(CustomGestureRecognizerTest, ShouldReturnNoMatchWhenNoGesturesExist) {
  CustomGestureRecognizer custom_gesture_recognizer;

  EXPECT_THAT(test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                                test::MakeLetterVGesturePath()),
              testing::IsEmpty());
}

TEST_F(CustomGestureRecognizerTest,
       ShouldReturnNoMatchForPathWithFewerThanTwoPoints) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                                {gfx::PointF()}),
              testing::IsEmpty());
}

TEST_F(CustomGestureRecognizerTest, ShouldMatchIdenticalPaths) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        test::MakeLetterVGesturePath()),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest,
       ShouldMatchPathsRegardlessOfSizeAndPosition) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath original_gesture_path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", original_gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // The same shape, scaled 2x and translated, not a hand-picked substitute.
  GesturePath scaled_and_moved_gesture_path;
  for (const gfx::PointF& point : original_gesture_path) {
    scaled_and_moved_gesture_path.emplace_back(point.x() * 2 + 100,
                                               point.y() * 2 + 50);
  }

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        scaled_and_moved_gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest,
       ShouldMatchSameShapeDespiteUnevenPointDensity) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath original_gesture_path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", original_gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // The same shape, traced with many duplicate points at the start;
  // arc-length resampling should normalize away the difference.
  GesturePath unevenly_sampled_gesture_path(10, original_gesture_path.front());
  unevenly_sampled_gesture_path.insert(unevenly_sampled_gesture_path.cend(),
                                       original_gesture_path.cbegin(),
                                       original_gesture_path.cend());

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        unevenly_sampled_gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldNotMatchDissimilarShapes) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // A straight horizontal line looks nothing like the V-shaped gesture.
  GesturePath straight_line_gesture_path = {gfx::PointF(),
                                            gfx::PointF(100.0, 0.0)};

  EXPECT_THAT(test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                                straight_line_gesture_path),
              testing::IsEmpty());
}

TEST_F(CustomGestureRecognizerTest, ShouldNotMatchRemovedGesture) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));

  custom_gesture_recognizer.Remove("id");

  EXPECT_THAT(test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                                test::MakeLetterVGesturePath()),
              testing::IsEmpty());
}

TEST_F(CustomGestureRecognizerTest, ShouldReplaceExistingEntryWithSameId) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kAnotherFakeGestureCommand)}));

  EXPECT_THAT(test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                                test::MakeLetterVGesturePath()),
              testing::ElementsAre(
                  MakeGestureAction(test::kAnotherFakeGestureCommand)));
  EXPECT_THAT(custom_gesture_recognizer.custom_gestures(), testing::SizeIs(1));
}

TEST_F(CustomGestureRecognizerTest,
       ShouldMatchClosedShapeFromDifferentStartingPoint) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath triangle_gesture_path = test::MakeTriangleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", triangle_gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // Same triangle, retraced starting partway along the gesture_path, since
  // a closed shape has no fixed starting point.
  const auto rotation_offset = base::checked_cast<GesturePath::difference_type>(
      triangle_gesture_path.size() / 2);
  GesturePath retraced_from_different_point_gesture_path(
      triangle_gesture_path.cbegin() + rotation_offset,
      triangle_gesture_path.cend());
  retraced_from_different_point_gesture_path.insert(
      retraced_from_different_point_gesture_path.cend(),
      triangle_gesture_path.cbegin(),
      triangle_gesture_path.cbegin() + rotation_offset + 1);
  EXPECT_THAT(
      test::CustomGestureActionsForPath(
          custom_gesture_recognizer,
          retraced_from_different_point_gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest,
       ShouldNotMatchOpenShapeFromDifferentStartingPoint) {
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath letter_v_gesture_path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", letter_v_gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  // The V shape is open, so retracing it from a different point produces
  // a different gesture_path and should not match.
  GesturePath started_from_second_point_gesture_path(
      letter_v_gesture_path.cbegin() + 1, letter_v_gesture_path.cend());
  started_from_second_point_gesture_path.push_back(
      letter_v_gesture_path.front());
  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        started_from_second_point_gesture_path),
      testing::IsEmpty());
}

TEST_F(CustomGestureRecognizerTest, ShouldReturnUrlForMatch) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {test::MakeNavigateToUrlGestureAction(GURL("https://brave.com"))}));

  EXPECT_THAT(test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                                test::MakeLetterVGesturePath()),
              testing::ElementsAre(test::MakeNavigateToUrlGestureAction(
                  GURL("https://brave.com"))));
}

TEST_F(CustomGestureRecognizerTest, ShouldReturnAllChainedActionsForMatch) {
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand),
       test::MakeNavigateToUrlGestureAction(GURL("https://brave.com"))}));

  EXPECT_THAT(test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                                test::MakeLetterVGesturePath()),
              testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand),
                                   test::MakeNavigateToUrlGestureAction(
                                       GURL("https://brave.com"))));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeTriangle) {
  // Pinned from a user's profile (triangle, "New tab in group"), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeTriangleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792208323001", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeZigzag) {
  // Pinned from a user's profile (zigzag, "Navigate to URL" brave.com), so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeZigzagGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792208750560", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeLetterV) {
  // Pinned from a user's profile (letter V, unassigned), so future changes
  // to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeLetterVGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212339556", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeConjunction) {
  // Pinned from a user's profile (conjunction, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeConjunctionGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212345544", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeCircle) {
  // Pinned from a user's profile (circle, unassigned), so future changes
  // to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeCircleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212354892", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeSpiral) {
  // Pinned from a user's profile (spiral/@, "Add new tab to group"), so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeSpiralGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212359358", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeHandwriting) {
  // Pinned from a user's profile (handwriting, "Open Settings Page"
  // /leo-ai), so future changes can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeHandwritingGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792212468887", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeHook) {
  // Pinned from a user's profile (hook, "About Brave VPN"), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeHookGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792217771449", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeRocket) {
  // Pinned from a user's profile (rocket, "About Brave"), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeRocketGesturePath();
  custom_gesture_recognizer.Add(
      test::MakeCustomGesture(/*id=*/"1792218580198", gesture_path,
                              {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeDiamond) {
  // Pinned from a user's profile (diamond, "New tab to the right"), so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeDiamondGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240640644", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeOval) {
  // Pinned from a user's profile (oval, unassigned), so future changes to
  // the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeOvalGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240667421", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeLeftOpenFrame) {
  // Pinned from a user's profile (left-open frame, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeLeftOpenFrameGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240836620", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeRightOpenFrame) {
  // Pinned from a user's profile (right-open frame, "Copy Clean Link"), so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeRightOpenFrameGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240839975", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeSquareArch) {
  // Pinned from a user's profile (square arch, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeSquareArchGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240843266", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeIntersection) {
  // Pinned from a user's profile (intersection, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeIntersectionGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240845970", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeLetterC) {
  // Pinned from a user's profile (letter C, "Copy"), so future changes to
  // the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeLetterCGesturePath();
  custom_gesture_recognizer.Add(
      test::MakeCustomGesture(/*id=*/"1792240848916", gesture_path,
                              {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeLetterU) {
  // Pinned from a user's profile (letter U, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeLetterUGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240851475", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeReversedC) {
  // Pinned from a user's profile (reversed C, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeReversedCGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240854497", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeAngledCorner) {
  // Pinned from a user's profile (angled corner, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeAngledCornerGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240860585", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeSquiggle) {
  // Pinned from a user's profile (squiggle, unassigned), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeSquiggleGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240876584", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeDoublePeak) {
  // Pinned from a user's profile (double peak, "Bookmark bar edit"), so
  // future changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeDoublePeakGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792240882328", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

TEST_F(CustomGestureRecognizerTest, ShouldRecognizeCoil) {
  // Pinned from a user's profile (coil, "Block elements"), so future
  // changes to the matching pipeline can't silently stop recognizing it.
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path = test::MakeCoilGesturePath();
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"1792241147635", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));

  EXPECT_THAT(
      test::CustomGestureActionsForPath(custom_gesture_recognizer,
                                        gesture_path),
      testing::ElementsAre(MakeGestureAction(test::kFakeGestureCommand)));
}

}  // namespace gestures
