/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_value_util.h"

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kSegmentsAsJson[] =
    R"(["technology & computing","personal finance-banking","food & drink-restaurants"])";
constexpr char kNoSegmentsAsJson[] = "[]";

}  // namespace

TEST(BraveAdsSegmentValueUtilTest, SegmentsToValue) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(
      base::test::ParseJsonList(kSegmentsAsJson),
      SegmentsToValue({"technology & computing", "personal finance-banking",
                       "food & drink-restaurants"}));
}

TEST(BraveAdsSegmentValueUtilTest, NoSegmentsToValue) {
  // Arrange

  // Act
  const base::Value::List value = SegmentsToValue({});

  // Assert
  EXPECT_TRUE(value.empty());
}

TEST(BraveAdsSegmentValueUtilTest, SegmentsFromValue) {
  // Arrange
  const base::Value::List value = base::test::ParseJsonList(kSegmentsAsJson);

  // Act

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "personal finance-banking",
                                         "food & drink-restaurants"};
  EXPECT_EQ(expected_segments, SegmentsFromValue(value));
}

TEST(BraveAdsSegmentValueUtilTest, NoSegmentsFromValue) {
  // Arrange
  const base::Value::List value = base::test::ParseJsonList(kNoSegmentsAsJson);

  // Act
  const SegmentList segments = SegmentsFromValue(value);

  // Assert
  EXPECT_TRUE(segments.empty());
}

}  // namespace brave_ads
