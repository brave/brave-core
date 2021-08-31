/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segments_json_writer.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace JSONWriter {

TEST(BatAdsSegmentsJsonWriterTest, Segments) {
  // Arrange
  const SegmentList segments = {"technology & computing",
                                "personal finance-banking",
                                "food & drink-restaurants"};

  // Act
  const std::string json = WriteSegments(segments);

  // Assert
  const std::string expected_json =
      R"(["technology & computing","personal finance-banking","food & drink-restaurants"])";
  EXPECT_EQ(expected_json, json);
}

TEST(BatAdsSegmentsJsonWriterTest, NoSegments) {
  // Arrange
  const SegmentList segments = {};

  // Act
  const std::string json = WriteSegments(segments);

  // Assert
  const std::string expected_json = R"([])";
  EXPECT_EQ(expected_json, json);
}

}  // namespace JSONWriter
}  // namespace ads
