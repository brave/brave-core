/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segments_util.h"

#include <string>

#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const char kCatalog[] = "catalog_with_multiple_campaigns.json";

}  // namespace

class BatAdsAdTargetingSegmentUtilTest : public UnitTestBase {
 protected:
  BatAdsAdTargetingSegmentUtilTest() = default;

  ~BatAdsAdTargetingSegmentUtilTest() override = default;
};

TEST_F(BatAdsAdTargetingSegmentUtilTest, GetSegmentsFromCatalog) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const SegmentList segments = GetSegments(catalog);

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "untargeted"};
  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest,
       GetParentSegmentFromParentChildSegment) {
  // Arrange
  const std::string segment = "technology & computing-software";

  // Act
  const std::string parent_segment = GetParentSegment(segment);

  // Assert
  const std::string expected_parent_segment = "technology & computing";

  EXPECT_EQ(expected_parent_segment, parent_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, GetParentSegmentFromParentSegment) {
  // Arrange
  const std::string segment = "technology & computing";

  // Act
  const std::string parent_segment = GetParentSegment(segment);

  // Assert
  const std::string expected_parent_segment = "technology & computing";

  EXPECT_EQ(expected_parent_segment, parent_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, GetParentSegments) {
  // Arrange
  const SegmentList segments = {"technology & computing-software",
                                "personal finance-personal finance",
                                "automobiles"};

  // Act
  const SegmentList parent_segments = GetParentSegments(segments);

  // Assert
  const SegmentList expected_parent_segments = {
      "technology & computing", "personal finance", "automobiles"};

  EXPECT_EQ(expected_parent_segments, parent_segments);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, GetParentSegmentsForEmptyList) {
  // Arrange
  const SegmentList segments;

  // Act
  const SegmentList parent_segments = GetParentSegments(segments);

  // Assert
  const SegmentList expected_parent_segments = {};

  EXPECT_EQ(expected_parent_segments, parent_segments);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest,
       ShouldFilterMatchingParentChildSegment) {
  // Arrange
  Client::Get()->ToggleAdOptOutAction("parent-child",
                                      CategoryContentInfo::OptAction::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent-child");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest,
       ShouldNotFilterNonMatchingParentChildSegment) {
  // Arrange
  Client::Get()->ToggleAdOptOutAction("parent-child",
                                      CategoryContentInfo::OptAction::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo-bar");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, ShouldFilterMatchingParentSegment) {
  // Arrange
  Client::Get()->ToggleAdOptOutAction("parent",
                                      CategoryContentInfo::OptAction::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest,
       ShouldNotFilterNonMatchingParentSegment) {
  // Arrange
  Client::Get()->ToggleAdOptOutAction("parent",
                                      CategoryContentInfo::OptAction::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest,
       ShouldFilterAgainstParentForMatchingParentSegmentWithChild) {
  // Arrange
  Client::Get()->ToggleAdOptOutAction("parent",
                                      CategoryContentInfo::OptAction::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent-child");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest,
       ShouldNotFilterAgainstParentForNonMatchingParentSegmentWithChild) {
  // Arrange
  Client::Get()->ToggleAdOptOutAction("parent",
                                      CategoryContentInfo::OptAction::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo-bar");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, ParentSegmentsMatch) {
  // Arrange

  // Act
  const bool does_match = ParentSegmentsMatch("technology & computing-windows",
                                              "technology & computing-linux");

  // Assert
  EXPECT_TRUE(does_match);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, ParentSegmentsDoNotMatch) {
  // Arrange

  // Act
  const bool does_match =
      ParentSegmentsMatch("business-banking", "technology & computing-linux");

  // Assert
  EXPECT_FALSE(does_match);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, HasChildSegment) {
  // Arrange

  // Act
  const bool has_child_segment =
      HasChildSegment("technology & computing-windows");

  // Assert
  EXPECT_TRUE(has_child_segment);
}

TEST_F(BatAdsAdTargetingSegmentUtilTest, DoesNotHaveChildSegment) {
  // Arrange

  // Act
  const bool has_child_segment = HasChildSegment("technology & computing");

  // Assert
  EXPECT_FALSE(has_child_segment);
}

}  // namespace ads
