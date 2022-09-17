/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segment_util.h"

#include "absl/types/optional.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "bat/ads/internal/catalog/catalog_json_reader.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCatalog[] = "catalog_with_multiple_campaigns.json";
}  // namespace

class BatAdsTargetingSegmentUtilTest : public UnitTestBase {};

TEST_F(BatAdsTargetingSegmentUtilTest, GetSegmentsFromCatalog) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(json);

  const absl::optional<CatalogInfo> catalog_info =
      json::reader::ReadCatalog(*json);
  ASSERT_TRUE(catalog_info);

  // Act
  const SegmentList segments = GetSegments(*catalog_info);

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "untargeted"};
  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTargetingSegmentUtilTest, GetSegmentsFromEmptyCatalog) {
  // Arrange
  CatalogInfo catalog;

  // Act
  const SegmentList segments = GetSegments(catalog);

  // Assert
  SegmentList expected_segments;
  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTargetingSegmentUtilTest, GetParentSegmentFromParentChildSegment) {
  // Arrange
  const std::string segment = "technology & computing-software";

  // Act
  const std::string parent_segment = GetParentSegment(segment);

  // Assert
  const std::string expected_parent_segment = "technology & computing";

  EXPECT_EQ(expected_parent_segment, parent_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest, GetParentSegmentFromParentSegment) {
  // Arrange
  const std::string segment = "technology & computing";

  // Act
  const std::string parent_segment = GetParentSegment(segment);

  // Assert
  const std::string expected_parent_segment = "technology & computing";

  EXPECT_EQ(expected_parent_segment, parent_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest, GetParentSegments) {
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

TEST_F(BatAdsTargetingSegmentUtilTest, GetParentSegmentsForEmptyList) {
  // Arrange
  const SegmentList segments;

  // Act
  const SegmentList parent_segments = GetParentSegments(segments);

  // Assert
  const SegmentList expected_parent_segments;

  EXPECT_EQ(expected_parent_segments, parent_segments);
}

TEST_F(BatAdsTargetingSegmentUtilTest, ShouldFilterMatchingParentChildSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleAdOptOut(
      "parent-child", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent-child");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest,
       ShouldNotFilterNonMatchingParentChildSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleAdOptOut(
      "parent-child", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo-bar");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest, ShouldFilterMatchingParentSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleAdOptOut(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest,
       ShouldNotFilterNonMatchingParentSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleAdOptOut(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest,
       ShouldFilterAgainstParentForMatchingParentSegmentWithChild) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleAdOptOut(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent-child");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest,
       ShouldNotFilterAgainstParentForNonMatchingParentSegmentWithChild) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleAdOptOut(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo-bar");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest, MatchParentSegments) {
  // Arrange

  // Act
  const bool does_match = MatchParentSegments("technology & computing-windows",
                                              "technology & computing-linux");

  // Assert
  EXPECT_TRUE(does_match);
}

TEST_F(BatAdsTargetingSegmentUtilTest, ParentSegmentsDoNotMatch) {
  // Arrange

  // Act
  const bool does_match =
      MatchParentSegments("business-banking", "technology & computing-linux");

  // Assert
  EXPECT_FALSE(does_match);
}

TEST_F(BatAdsTargetingSegmentUtilTest, HasChildSegment) {
  // Arrange

  // Act
  const bool has_child_segment =
      HasChildSegment("technology & computing-windows");

  // Assert
  EXPECT_TRUE(has_child_segment);
}

TEST_F(BatAdsTargetingSegmentUtilTest, DoesNotHaveChildSegment) {
  // Arrange

  // Act
  const bool has_child_segment = HasChildSegment("technology & computing");

  // Assert
  EXPECT_FALSE(has_child_segment);
}

}  // namespace ads
