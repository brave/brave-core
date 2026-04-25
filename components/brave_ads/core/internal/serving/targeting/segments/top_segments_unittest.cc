/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_segments.h"

#include <cstddef>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr size_t kSegmentsMaxCount = 2;
}  // namespace

class BraveAdsTopSegmentsTest : public test::TestBase {};

TEST_F(BraveAdsTopSegmentsTest, GetTopChildSegments) {
  // Arrange
  const SegmentList segments = {"parent_1-child", "parent_2-child",
                                "parent_3-child"};

  // Act
  const SegmentList top_segments = GetTopSegments(segments, kSegmentsMaxCount,
                                                  /*parent_only=*/false);

  // Assert
  const SegmentList expected_top_segments = {"parent_1-child",
                                             "parent_2-child"};
  EXPECT_EQ(expected_top_segments, top_segments);
}

TEST_F(BraveAdsTopSegmentsTest, GetEmptyTopChildSegments) {
  // Act
  const SegmentList top_segments = GetTopSegments(
      /*segments=*/{}, kSegmentsMaxCount, /*parent_only=*/false);

  // Assert
  EXPECT_THAT(top_segments, ::testing::IsEmpty());
}

TEST_F(BraveAdsTopSegmentsTest, GetTopParentSegments) {
  // Arrange
  const SegmentList segments = {"parent_1-child", "parent_2-child",
                                "parent_3-child"};

  // Act
  const SegmentList top_segments = GetTopSegments(segments, kSegmentsMaxCount,
                                                  /*parent_only=*/true);

  // Assert
  const SegmentList expected_top_segments = {"parent_1", "parent_2"};
  EXPECT_EQ(expected_top_segments, top_segments);
}

TEST_F(BraveAdsTopSegmentsTest, GetEmptyTopParentSegments) {
  // Act
  const SegmentList top_segments = GetTopSegments(
      /*segments=*/{}, kSegmentsMaxCount, /*parent_only=*/false);

  // Assert
  EXPECT_THAT(top_segments, ::testing::IsEmpty());
}

}  // namespace brave_ads
