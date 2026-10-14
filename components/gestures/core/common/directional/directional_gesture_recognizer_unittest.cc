// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"

#include <cmath>
#include <numbers>

#include "base/numerics/safe_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureRecognizerTest, ShouldIgnoreMovementBelowThreshold) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(10, 0),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(), testing::IsEmpty());
}

TEST(DirectionalGestureRecognizerTest,
     ShouldWaitForConsistentDirectionBeforeFirstStroke) {
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

TEST(DirectionalGestureRecognizerTest,
     ShouldRecognizeStrokeAtConfiguredThreshold) {
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
              testing::ElementsAre(DirectionalGestureStrokeType::kRight));
}

TEST(DirectionalGestureRecognizerTest,
     ShouldRecognizeSecondStrokeAfterSustainedDirectionChange) {
  // A clean 90-degree turn following an already-committed stroke must
  // commit as its own stroke, not get absorbed into the first one.
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/30,
                           /*min_stroke_straightness=*/0.9);
  for (const gfx::PointF& location :
       {gfx::PointF(10, 0), gfx::PointF(20, 0), gfx::PointF(30, 0),
        gfx::PointF(40, 0), gfx::PointF(50, 0), gfx::PointF(60, 0),
        gfx::PointF(70, 0), gfx::PointF(80, 0), gfx::PointF(90, 0),
        gfx::PointF(100, 0)}) {
    gesture_recognizer.OnMouseMoved(location, /*now=*/base::TimeTicks());
  }
  ASSERT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureStrokeType::kRight));

  for (const gfx::PointF& location :
       {gfx::PointF(100, 10), gfx::PointF(100, 20), gfx::PointF(100, 30),
        gfx::PointF(100, 40), gfx::PointF(100, 50), gfx::PointF(100, 60)}) {
    gesture_recognizer.OnMouseMoved(location, /*now=*/base::TimeTicks());
  }

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureStrokeType::kRight,
                                   DirectionalGestureStrokeType::kDown));
}

TEST(DirectionalGestureRecognizerTest,
     ShouldNotRecognizeWanderingCurveAsStroke) {
  // A path that keeps changing candidate direction before any one of them
  // sustains must not let a short, locally-straight tail "launder" the
  // curve that preceded it.
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/30,
                           /*min_stroke_straightness=*/0.9);
  for (const gfx::PointF& location :
       {gfx::PointF(10, 2), gfx::PointF(18, 8), gfx::PointF(22, 18),
        gfx::PointF(20, 28), gfx::PointF(12, 35), gfx::PointF(2, 36),
        gfx::PointF(-6, 30), gfx::PointF(-8, 20), gfx::PointF(-4, 10),
        gfx::PointF(10, 8), gfx::PointF(25, 8), gfx::PointF(40, 8)}) {
    gesture_recognizer.OnMouseMoved(location, /*now=*/base::TimeTicks());
  }

  EXPECT_THAT(gesture_recognizer.strokes(), testing::IsEmpty());
}

TEST(DirectionalGestureRecognizerTest,
     ShouldRecognizeStraightDiagonalMovement) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(20, 20),
                                  /*now=*/base::TimeTicks());  // 45 degrees.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(40, 40),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureStrokeType::kDownRight));
}

TEST(DirectionalGestureRecognizerTest, ShouldChooseNearestOfEightDirections) {
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
              testing::ElementsAre(DirectionalGestureStrokeType::kRight));
}

TEST(DirectionalGestureRecognizerTest, ShouldRecognizeVerticalMovement) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 61),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 122),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureStrokeType::kDown));
}

TEST(DirectionalGestureRecognizerTest,
     ShouldCombineConsecutiveDuplicateStrokes) {
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
              testing::ElementsAre(DirectionalGestureStrokeType::kDown));
}

TEST(DirectionalGestureRecognizerTest,
     ShouldPreserveDifferentConsecutiveStrokes) {
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
              testing::ElementsAre(DirectionalGestureStrokeType::kDown,
                                   DirectionalGestureStrokeType::kRight));
}

TEST(DirectionalGestureRecognizerTest,
     ShouldIgnoreBriefDiagonalMovementDuringTurn) {
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
              testing::ElementsAre(DirectionalGestureStrokeType::kDown,
                                   DirectionalGestureStrokeType::kRight));
}

TEST(DirectionalGestureRecognizerTest,
     ShouldNotTreatContinuousCurveAsSingleStroke) {
  // A hand-drawn oval/circle traces a smoothly, continuously turning path.
  // Since no single direction ever sustains on its own for a full
  // `min_stroke_pixels` before the curve has already moved on to the next
  // octant, no stroke should ever commit.
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(100, 100),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/30);
  constexpr int kPointCount = 16;
  constexpr float kRadius = 20;
  for (int i = 1; i <= kPointCount; ++i) {
    const float angle = 2 * std::numbers::pi_v<float> *
                        base::saturated_cast<float>(i) / kPointCount;
    gesture_recognizer.OnMouseMoved(
        /*location=*/gfx::PointF(100 + kRadius * std::cos(angle),
                                 100 + kRadius * std::sin(angle)),
        /*now=*/base::TimeTicks());
  }

  EXPECT_THAT(gesture_recognizer.strokes(), testing::IsEmpty());
}

TEST(DirectionalGestureRecognizerTest,
     ShouldDiscardStrokesBeyondMaximumLength) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature,
      {{features::kMaxDirectionalGestureStrokes.name, "2"}});
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
  // A third stroke's worth of movement, discarded, since the cap of 2 is
  // already reached, so `OnMouseMoved` returns before even considering it.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 0),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureStrokeType::kDown,
                                   DirectionalGestureStrokeType::kRight));
}

TEST(DirectionalGestureRecognizerTest, ShouldNotExtendStrokeWhenPathLoopsBack) {
  // After a `kRight` stroke commits, a loop that happens to swing back
  // through the `kRight` octant relative to the stroke's origin must not be
  // accepted as "continuing" that stroke just because it lands in the same
  // direction, since the loop's own arc length must still be charged
  // against straightness, same as any other divergence. The long, clean
  // `kDown` run afterwards has to be long enough to outweigh the loop's
  // wander, which is
  // charged against it via `departure_origin_` the same way it would be for
  // any other stroke built on top of a rejected divergence.
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/30,
                           /*min_stroke_straightness=*/0.9);
  for (const gfx::PointF& location :
       {gfx::PointF(30, 0), gfx::PointF(60, 0), gfx::PointF(60, 50),
        gfx::PointF(110, 50), gfx::PointF(90, 0), gfx::PointF(90, 1000),
        gfx::PointF(90, 1500), gfx::PointF(90, 2000)}) {
    gesture_recognizer.OnMouseMoved(location, /*now=*/base::TimeTicks());
  }

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureStrokeType::kRight,
                                   DirectionalGestureStrokeType::kDown));
}

TEST(DirectionalGestureRecognizerTest, ShouldNotTimeOutBeforeHoldTimeout) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature,
      {{features::kGestureSessionIdleHoldTimeout.name, "3s"}});
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks());

  EXPECT_FALSE(gesture_recognizer.TimedOut(/*now=*/base::TimeTicks() +
                                           base::Seconds(2)));
}

TEST(DirectionalGestureRecognizerTest,
     ShouldTimeOutAfterHoldTimeoutWithoutMovement) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature,
      {{features::kGestureSessionIdleHoldTimeout.name, "3s"}});
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks());

  EXPECT_TRUE(gesture_recognizer.TimedOut(/*now=*/base::TimeTicks() +
                                          base::Seconds(3)));
}

TEST(DirectionalGestureRecognizerTest, ShouldNotTimeOutWhileMovementContinues) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kGesturesFeature,
      {{features::kGestureSessionIdleHoldTimeout.name, "3s"}});
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks());
  // Qualifying movement resets the idle timer even though it only proposes a
  // pending candidate direction here, rather than committing a stroke.
  gesture_recognizer.OnMouseMoved(
      /*location=*/gfx::PointF(0, 30),
      /*now=*/base::TimeTicks() + base::Seconds(2));

  EXPECT_FALSE(gesture_recognizer.TimedOut(
      /*now=*/base::TimeTicks() + base::Seconds(4)));
}

}  // namespace gestures
