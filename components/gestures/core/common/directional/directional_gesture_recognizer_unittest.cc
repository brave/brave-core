// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"

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

TEST(DirectionalGestureRecognizerTest, RegistersStrokeAtConfiguredThreshold) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(20, 0),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kRight));
}

TEST(DirectionalGestureRecognizerTest, ResolvesStraightDiagonalMovement) {
  DirectionalGestureRecognizer gesture_recognizer;

  gesture_recognizer.Reset(/*start_location=*/gfx::PointF(),
                           /*now=*/base::TimeTicks(),
                           /*min_stroke_pixels=*/20);
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(40, 40),
                                  /*now=*/base::TimeTicks());  // 45 degrees.

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
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(0, 30),
                                  /*now=*/base::TimeTicks());
  // A direction change is only committed once sustained on its own; the
  // first sample proposes the new direction, the second confirms it.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(30, 30),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 30),
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
                                  /*now=*/base::TimeTicks());  // Down.
  // Briefly moves through the diagonal octant while turning, as a smooth
  // 90-degree turn naturally does, rather than a sharp right angle.
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(25, 45),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 40),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(90, 35),
                                  /*now=*/base::TimeTicks());

  EXPECT_THAT(gesture_recognizer.strokes(),
              testing::ElementsAre(DirectionalGestureDirectionType::kDown,
                                   DirectionalGestureDirectionType::kRight));
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
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(30, 30),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(60, 30),
                                  /*now=*/base::TimeTicks());
  gesture_recognizer.OnMouseMoved(/*location=*/gfx::PointF(90, 0),
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
  gesture_recognizer.OnMouseMoved(
      /*location=*/gfx::PointF(0, 30),
      /*now=*/base::TimeTicks() + base::Seconds(2));

  EXPECT_FALSE(
      gesture_recognizer.TimedOut(base::TimeTicks() + base::Seconds(4)));
}

}  // namespace gestures
