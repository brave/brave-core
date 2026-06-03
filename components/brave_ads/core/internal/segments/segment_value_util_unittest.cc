/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_value_util.h"

#include <string_view>

#include "base/test/values_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr std::string_view kSegmentsAsJson =
    R"JSON(
        [
          "technology & computing",
          "personal finance-banking",
          "food & drink-restaurants"
        ])JSON";

}  // namespace

TEST(BraveAdsSegmentValueUtilTest, SegmentsToList) {
  EXPECT_EQ(
      base::test::ParseJsonList(kSegmentsAsJson),
      SegmentsToList({"technology & computing", "personal finance-banking",
                      "food & drink-restaurants"}));
}

TEST(BraveAdsSegmentValueUtilTest, EmptySegmentsToList) {
  EXPECT_THAT(SegmentsToList({}), ::testing::IsEmpty());
}

TEST(BraveAdsSegmentValueUtilTest, SegmentsFromList) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(kSegmentsAsJson);

  // Act
  const SegmentList segments = SegmentsFromList(list);

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "personal finance-banking",
                                         "food & drink-restaurants"};
  EXPECT_EQ(expected_segments, segments);
}

TEST(BraveAdsSegmentValueUtilTest, NoSegmentsFromEmptyList) {
  // Act & Assert
  EXPECT_THAT(SegmentsFromList({}), ::testing::IsEmpty());
}

}  // namespace brave_ads
