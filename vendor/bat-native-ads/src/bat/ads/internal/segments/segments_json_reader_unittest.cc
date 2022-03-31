/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segments_json_reader.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace JSONReader {

TEST(BatAdsSegmentsJsonReaderTest, ValidJson) {
  // Arrange
  const std::string json =
      R"(["technology & computing","personal finance-banking","food & drink-restaurants"])";

  // Act
  const SegmentList segments = ReadSegments(json);

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "personal finance-banking",
                                         "food & drink-restaurants"};
  EXPECT_EQ(expected_segments, segments);
}

TEST(BatAdsSegmentsJsonReaderTest, InvalidJson) {
  // Arrange
  const std::string json = R"({FOOBAR})";

  // Act
  const SegmentList segments = ReadSegments(json);

  // Assert
  const SegmentList expected_segments = {};
  EXPECT_EQ(expected_segments, segments);
}

}  // namespace JSONReader
}  // namespace ads
