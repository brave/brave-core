/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_value_util.h"

#include "base/test/values_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kSegmentsAsJson[] =
    R"(
        [
          "technology & computing",
          "personal finance-banking",
          "food & drink-restaurants"
        ])";

}  // namespace

TEST(BraveAdsSegmentValueUtilTest, SegmentsToValue) {
  // Act
  const base::Value::List list =
      SegmentsToValue({"technology & computing", "personal finance-banking",
                       "food & drink-restaurants"});

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kSegmentsAsJson), list);
}

TEST(BraveAdsSegmentValueUtilTest, EmptySegmentsToValue) {
  // Act
  const base::Value::List list = SegmentsToValue({});

  // Assert
  EXPECT_THAT(list, ::testing::IsEmpty());
}

TEST(BraveAdsSegmentValueUtilTest, SegmentsFromValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList(kSegmentsAsJson);

  // Act
  const SegmentList segments = SegmentsFromValue(list);

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "personal finance-banking",
                                         "food & drink-restaurants"};
  EXPECT_EQ(expected_segments, segments);
}

TEST(BraveAdsSegmentValueUtilTest, EmptySegmentsFromValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList("[]");

  // Act
  const SegmentList segments = SegmentsFromValue(list);

  // Assert
  EXPECT_THAT(segments, ::testing::IsEmpty());
}

}  // namespace brave_ads
