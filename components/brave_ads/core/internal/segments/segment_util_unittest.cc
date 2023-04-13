/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_json_reader.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {
constexpr char kCatalog[] = "catalog_with_multiple_campaigns.json";
}  // namespace

class BatAdsSegmentUtilTest : public UnitTestBase {};

TEST_F(BatAdsSegmentUtilTest, GetSegmentsFromCatalog) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(json);

  const absl::optional<CatalogInfo> catalog = json::reader::ReadCatalog(*json);
  ASSERT_TRUE(catalog);

  // Act
  const SegmentList segments = GetSegments(*catalog);

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "untargeted"};
  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsSegmentUtilTest, GetSegmentsFromEmptyCatalog) {
  // Arrange

  // Act
  const SegmentList segments = GetSegments({});

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsSegmentUtilTest, GetParentSegmentFromParentChildSegment) {
  // Arrange

  // Act
  const std::string parent_segment =
      GetParentSegment("technology & computing-software");

  // Assert
  EXPECT_EQ("technology & computing", parent_segment);
}

TEST_F(BatAdsSegmentUtilTest, GetParentSegmentFromParentSegment) {
  // Arrange

  // Act
  const std::string parent_segment = GetParentSegment("technology & computing");

  // Assert
  EXPECT_EQ("technology & computing", parent_segment);
}

TEST_F(BatAdsSegmentUtilTest, GetParentSegments) {
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

TEST_F(BatAdsSegmentUtilTest, GetParentSegmentsForEmptyList) {
  // Arrange

  // Act
  const SegmentList parent_segments = GetParentSegments({});

  // Assert
  EXPECT_TRUE(parent_segments.empty());
}

TEST_F(BatAdsSegmentUtilTest, ShouldFilterMatchingParentChildSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleMarkToNoLongerReceiveAdsForCategory(
      "parent-child", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent-child");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentChildSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleMarkToNoLongerReceiveAdsForCategory(
      "parent-child", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo-bar");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsSegmentUtilTest, ShouldFilterMatchingParentSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleMarkToNoLongerReceiveAdsForCategory(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentSegment) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleMarkToNoLongerReceiveAdsForCategory(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsSegmentUtilTest,
       ShouldFilterAgainstParentForMatchingParentSegmentWithChild) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleMarkToNoLongerReceiveAdsForCategory(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("parent-child");

  // Assert
  EXPECT_TRUE(should_filter_segment);
}

TEST_F(BatAdsSegmentUtilTest,
       ShouldNotFilterAgainstParentForNonMatchingParentSegmentWithChild) {
  // Arrange
  ClientStateManager::GetInstance()->ToggleMarkToNoLongerReceiveAdsForCategory(
      "parent", CategoryContentOptActionType::kNone);

  // Act
  const bool should_filter_segment = ShouldFilterSegment("foo-bar");

  // Assert
  EXPECT_FALSE(should_filter_segment);
}

TEST_F(BatAdsSegmentUtilTest, MatchParentSegments) {
  // Arrange

  // Act
  const bool does_match = MatchParentSegments("technology & computing-windows",
                                              "technology & computing-linux");

  // Assert
  EXPECT_TRUE(does_match);
}

TEST_F(BatAdsSegmentUtilTest, ParentSegmentsDoNotMatch) {
  // Arrange

  // Act
  const bool does_match =
      MatchParentSegments("business-banking", "technology & computing-linux");

  // Assert
  EXPECT_FALSE(does_match);
}

TEST_F(BatAdsSegmentUtilTest, HasChildSegment) {
  // Arrange

  // Act
  const bool has_child_segment =
      HasChildSegment("technology & computing-windows");

  // Assert
  EXPECT_TRUE(has_child_segment);
}

TEST_F(BatAdsSegmentUtilTest, DoesNotHaveChildSegment) {
  // Arrange

  // Act
  const bool has_child_segment = HasChildSegment("technology & computing");

  // Assert
  EXPECT_FALSE(has_child_segment);
}

}  // namespace brave_ads
