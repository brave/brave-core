/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/screenshot/core/browser/utils.h"

#include <utility>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace screenshot {

// Deliberately no base::DiscardableMemoryAllocator instance: ScaleDownBitmap()
// must not depend on one, because iOS does not install one and CHECK-fails if
// something asks for it (brave-core #35878).
TEST(ScreenshotUtilsUnitTest, ScaleDownBitmap) {
  // Oversized bitmaps are scaled to contain within 1024x768, preserving the
  // source aspect ratio, so only one dimension reaches the target.
  struct {
    int width;
    int height;
    int expected_width;
    int expected_height;
  } const large_test_dimensions[] = {
      {2560, 1440, 1024, 576},  // 16:9, fits to width.
      {1024, 1440, 546, 768},   // Taller than wide, fits to height.
      {2560, 768, 1024, 307},   // Wider than the target, fits to width.
      {5000, 1, 1024, 1},       // Extreme aspect ratios keep a single pixel.
      {1, 5000, 1, 768},
      // Only the height exceeds the target, so the result must be smaller than
      // the source in both dimensions rather than upscaled to reach 1024.
      {900, 1000, 691, 768},
  };
  for (const auto& [width, height, expected_width, expected_height] :
       large_test_dimensions) {
    SCOPED_TRACE(testing::Message() << width << "x" << height);
    const auto bitmap = gfx::test::CreateBitmap(width, height);
    const auto scaled_bitmap = ScaleDownBitmap(bitmap);
    EXPECT_EQ(scaled_bitmap.width(), expected_width);
    EXPECT_EQ(scaled_bitmap.height(), expected_height);
  }

  const std::vector<std::pair<int, int>> no_change_test_dimensions = {
      {1024, 768}, {1024, 720}, {960, 768}, {960, 720}};
  for (auto& [width, height] : no_change_test_dimensions) {
    SCOPED_TRACE(testing::Message() << width << "x" << height);
    const auto bitmap = gfx::test::CreateBitmap(width, height);
    const auto scaled_bitmap = ScaleDownBitmap(bitmap);
    EXPECT_TRUE(gfx::test::AreBitmapsEqual(bitmap, scaled_bitmap));
  }
}

}  // namespace screenshot
