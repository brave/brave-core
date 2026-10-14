// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_canonical_path_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureCanonicalPathUtilTest,
     ShouldBuildTemplateWithOneUnitSegmentPerStroke) {
  EXPECT_THAT(
      DirectionalGestureStrokesToCanonicalPath(
          DirectionalGestureStrokes{DirectionalGestureStrokeType::kRight,
                                    DirectionalGestureStrokeType::kDown}),
      testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(1, 0),
                           gfx::PointF(1, 1)));
}

TEST(DirectionalGestureCanonicalPathUtilTest,
     ShouldReturnOriginForEmptyStrokeTemplate) {
  EXPECT_THAT(
      DirectionalGestureStrokesToCanonicalPath(DirectionalGestureStrokes{}),
      testing::ElementsAre(gfx::PointF(0, 0)));
}

}  // namespace gestures
