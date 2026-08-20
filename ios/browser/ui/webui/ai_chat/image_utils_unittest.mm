// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/image_utils.h"

#import <Foundation/Foundation.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "skia/ext/skia_utils_ios.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace ai_chat {

namespace {

// Encoded bytes for a `width`x`height` image, standing in for the file the
// WebUI hands the page handler.
std::vector<uint8_t> CreateEncodedImage(int width, int height) {
  std::optional<std::vector<uint8_t>> encoded =
      gfx::PNGCodec::EncodeBGRASkBitmap(gfx::test::CreateBitmap(width, height),
                                        /*discard_transparency=*/false);
  CHECK(encoded);
  return std::move(*encoded);
}

// Decodes `encoded` back to a bitmap so its dimensions can be asserted on.
SkBitmap Decode(const std::vector<uint8_t>& encoded) {
  const std::vector<SkBitmap> bitmaps =
      skia::ImageDataToSkBitmaps([NSData dataWithBytes:encoded.data()
                                                length:encoded.size()]);
  return bitmaps.empty() ? SkBitmap() : bitmaps.front();
}

}  // namespace

using AIChatImageUtilsTest = PlatformTest;

// Oversized images are scaled to fit within 1024x768 with their aspect ratio
// preserved, so exactly one dimension reaches the bounds. This used to scale
// with -imageByPreparingThumbnailOfSize:, which instead fit the longest edge to
// 1024: the taller-than-4:3 cases came back up to 1024 tall, over the bounds.
TEST_F(AIChatImageUtilsTest, ScalesOversizedImagesToFitBounds) {
  struct {
    int width;
    int height;
    int expected_width;
    int expected_height;
  } const kCases[] = {
      {2560, 1440, 1024, 576},  // 16:9, fits to width.
      {2560, 768, 1024, 307},   // Wider than the bounds, fits to width.
      {1024, 1440, 546, 768},   // Taller than 4:3, fits to height.
      {768, 2000, 295, 768},    // Taller than 4:3, fits to height.
  };
  for (const auto& [width, height, expected_width, expected_height] : kCases) {
    SCOPED_TRACE(testing::Message() << width << "x" << height);
    const std::optional<std::vector<uint8_t>> scaled =
        DecodeAndScaleImageData(CreateEncodedImage(width, height));
    ASSERT_TRUE(scaled);
    const SkBitmap bitmap = Decode(*scaled);
    EXPECT_EQ(bitmap.width(), expected_width);
    EXPECT_EQ(bitmap.height(), expected_height);
  }
}

// An image that exceeds only one bound must still come back smaller than it
// went in. -imageByPreparingThumbnailOfSize: scaled these UP to reach 1024 on
// their longest edge (900x1000 became 921x1024), inflating the upload it was
// meant to shrink.
TEST_F(AIChatImageUtilsTest, NeverUpscales) {
  const std::vector<std::pair<int, int>> kDimensions = {{900, 1000},
                                                        {1000, 900}};
  for (const auto& [width, height] : kDimensions) {
    SCOPED_TRACE(testing::Message() << width << "x" << height);
    const std::optional<std::vector<uint8_t>> scaled =
        DecodeAndScaleImageData(CreateEncodedImage(width, height));
    ASSERT_TRUE(scaled);
    const SkBitmap bitmap = Decode(*scaled);
    EXPECT_LE(bitmap.width(), width);
    EXPECT_LE(bitmap.height(), height);
    EXPECT_LE(bitmap.width(), 1024);
    EXPECT_LE(bitmap.height(), 768);
  }
}

TEST_F(AIChatImageUtilsTest, LeavesImagesWithinBoundsAtTheirOriginalSize) {
  const std::vector<std::pair<int, int>> kDimensions = {
      {1024, 768}, {1024, 720}, {960, 768}, {640, 480}};
  for (const auto& [width, height] : kDimensions) {
    SCOPED_TRACE(testing::Message() << width << "x" << height);
    const std::optional<std::vector<uint8_t>> scaled =
        DecodeAndScaleImageData(CreateEncodedImage(width, height));
    ASSERT_TRUE(scaled);
    const SkBitmap bitmap = Decode(*scaled);
    EXPECT_EQ(bitmap.width(), width);
    EXPECT_EQ(bitmap.height(), height);
  }
}

TEST_F(AIChatImageUtilsTest, ReturnsNulloptForUndecodableData) {
  EXPECT_FALSE(DecodeAndScaleImageData({}));

  const std::string not_an_image = "this is not an image";
  EXPECT_FALSE(DecodeAndScaleImageData(
      std::vector<uint8_t>(not_an_image.begin(), not_an_image.end())));
}

}  // namespace ai_chat
