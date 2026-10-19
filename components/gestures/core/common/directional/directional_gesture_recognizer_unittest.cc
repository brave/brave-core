// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"

#include <cmath>
#include <numbers>

#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureRecognizerTest, IgnoresMovementBelowThreshold) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(10, 0),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(), testing::IsEmpty());
}

TEST(DirectionalGestureRecognizerTest,
     ProposedFirstDirectionDoesNotCommitUntilSustained) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(20, 0),
                                  /*now=*/base::TimeTicks());

  // Unlike every later stroke, the very first qualifying direction used to
  // commit unconditionally here, with no sustain check at all; that
  // asymmetry is what let a hand-drawn oval collapse to a single false
  // stroke.
  EXPECT_THAT(gesture_recognizer.strokes(), testing::IsEmpty());
}

TEST(DirectionalGestureRecognizerTest, RegistersStrokeAtConfiguredThreshold) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(20, 0),
                                  /*now=*/base::TimeTicks());
  // Confirms the proposed direction by sustaining it on its own for another
  // `min_stroke_pixels`.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(40, 0),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kRight));
}

TEST(DirectionalGestureRecognizerTest, ResolvesStraightDiagonalMovement) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(20, 20),
                                  /*now=*/base::TimeTicks());  // 45 degrees.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(40, 40),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(
      gesture_recognizer.strokes(),
      testing::ElementsAre(DirectionalGestureDirectionType::kDownRight));
}

TEST(DirectionalGestureRecognizerTest, ResolvesToNearestOfEightDirections) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(
      /*location=*/gfx::PointF(40, 10),
      /*now=*/base::TimeTicks());  // ~14 degrees.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(80, 20),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kRight));
}

TEST(DirectionalGestureRecognizerTest, ResolvesVerticalMovement) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 61),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 122),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kDown));
}

TEST(DirectionalGestureRecognizerTest, CollapsesConsecutiveDuplicateStrokes) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 30),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 60),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 90),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kDown));
}

TEST(DirectionalGestureRecognizerTest, RecordsDistinctConsecutiveStrokes) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  // The first sample proposes `kDown`, the second confirms and commits it.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 30),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 60),
                                  /*now=*/base::TimeTicks());
  // Likewise, the third sample proposes `kRight`, the fourth confirms it.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(30, 60),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 60),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kDown,
                                   DirectionalGestureDirectionType::kRight));
}

TEST(DirectionalGestureRecognizerTest,
     IgnoresTransientDiagonalBlipWhileTurningBetweenStrokes) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 30),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 60),
                                  /*now=*/base::TimeTicks());  // Commits down.
  // Briefly moves through the diagonal octant while turning, as a smooth
  // 90-degree turn naturally does, rather than a sharp right angle.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(25, 85),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 80),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(90, 85),
                                  /*now=*/base::TimeTicks());  // Commits right.

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kDown,
                                   DirectionalGestureDirectionType::kRight));
}

TEST(DirectionalGestureRecognizerTest,
     DoesNotCollapseAContinuouslyCurvingPathToASingleFalseStroke) {
  // Regression test: a hand-drawn oval/circle traces a smoothly, continuously
  // turning path. Since no single direction ever sustains on its own for a
  // full `min_stroke_pixels` before the curve has already moved on to the
  // next octant, no stroke should ever commit.
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(100, 100),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/30);
  constexpr int kPointCount = 16;
  constexpr float kRadius = 20;
  for (int i = 1; i <= kPointCount; ++i) {
    const float angle = 2 * std::numbers::pi_v<float> * i / kPointCount;
    gesture_recognizer.OnMouseMoved(
        gfx::PointF(100 + kRadius * std::cos(angle),
                    100 + kRadius * std::sin(angle)),
        base::TimeTicks());
  }

  EXPECT_THAT(gesture_recognizer.strokes(), testing::IsEmpty());
}

TEST(DirectionalGestureRecognizerTest, DiscardsExcessStrokesBeyondMaxLength) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature,
      {{features::kMaxDirectionalStrokes.name, "2"}});
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 30),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 60),
                                  /*now=*/base::TimeTicks());  // Commits down.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(30, 60),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 60),
                                  /*now=*/base::TimeTicks());  // Commits right.
  // A third stroke's worth of movement, discarded: the cap of 2 is already
  // reached, so `OnMouseMoved` returns before even considering it.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 0),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kDown,
                                   DirectionalGestureDirectionType::kRight));
}

TEST(DirectionalGestureRecognizerTest, DoesNotTimeOutBeforeHoldTimeoutElapses) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature, {{features::kIdleHoldTimeout.name, "3s"}});
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks());

  EXPECT_FALSE(
      gesture_recognizer.TimedOut(base::TimeTicks() + base::Seconds(2)));
}

TEST(DirectionalGestureRecognizerTest, TimesOutAfterHoldTimeoutWithNoMovement) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature, {{features::kIdleHoldTimeout.name, "3s"}});
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks());

  EXPECT_TRUE(
      gesture_recognizer.TimedOut(base::TimeTicks() + base::Seconds(3)));
}

TEST(DirectionalGestureRecognizerTest, DoesNotTimeOutWhileMovementContinues) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature, {{features::kIdleHoldTimeout.name, "3s"}});
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks());
  // Qualifying movement resets the idle timer even though it only proposes a
  // pending candidate direction here, rather than committing a stroke.
  gesture_recognizer.OnMouseMoved(
      /*location=*/gfx::PointF(0, 30),
      /*now=*/base::TimeTicks() + base::Seconds(2));

  EXPECT_FALSE(
      gesture_recognizer.TimedOut(base::TimeTicks() + base::Seconds(4)));
}

}  // namespace gestures
