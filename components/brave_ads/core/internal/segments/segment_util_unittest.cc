/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_json_reader.h"
#include "brave/components/brave_ads/core/internal/common/test/file_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSegmentUtilTest : public test::TestBase {};

TEST_F(BraveAdsSegmentUtilTest, GetSegmentsFromCatalog) {
  // Arrange
  const std::optional<std::string> contents = test::MaybeReadFileToString(
      test::kCatalogWithMultipleCampaignsJsonFilename);
  ASSERT_TRUE(contents);

  const std::optional<CatalogInfo> catalog =
      json::reader::ReadCatalog(*contents);
  ASSERT_TRUE(catalog);

  // Act
  const SegmentList segments = GetSegments(*catalog);

  // Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "untargeted"};
  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BraveAdsSegmentUtilTest, GetSegmentsFromEmptyCatalog) {
  // Act & Assert
  EXPECT_THAT(GetSegments(/*catalog=*/{}), ::testing::IsEmpty());
}

TEST_F(BraveAdsSegmentUtilTest, GetParentSegmentFromParentChildSegment) {
  // Act & Assert
  EXPECT_EQ("technology & computing",
            GetParentSegment("technology & computing-software"));
}

TEST_F(BraveAdsSegmentUtilTest, GetParentSegmentFromParentSegment) {
  // Act & Assert
  EXPECT_EQ("technology & computing",
            GetParentSegment("technology & computing"));
}

TEST_F(BraveAdsSegmentUtilTest, GetParentSegments) {
  // Arrange
  const SegmentList segments = {"technology & computing-software",
                                "personal finance-personal finance",
                                "automotive"};

  // Act
  const SegmentList parent_segments = GetParentSegments(segments);

  // Assert
  const SegmentList expected_parent_segments = {
      "technology & computing", "personal finance", "automotive"};
  EXPECT_EQ(expected_parent_segments, parent_segments);
}

TEST_F(BraveAdsSegmentUtilTest, GetParentSegmentsForEmptyList) {
  // Act & Assert
  EXPECT_THAT(GetParentSegments(/*segments=*/{}), ::testing::IsEmpty());
}

TEST_F(BraveAdsSegmentUtilTest, ShouldFilterMatchingParentChildSegment) {
  // Arrange
  GetReactions().ToggleDislikeSegment("parent-child");

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent-child"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentChildSegment) {
  // Arrange
  GetReactions().ToggleDislikeSegment("parent-child");

  // Act & Assert
  EXPECT_FALSE(ShouldFilterSegment("foo-bar"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldFilterMatchingParentSegment) {
  // Arrange
  GetReactions().ToggleDislikeSegment("parent");

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentSegment) {
  // Arrange
  GetReactions().ToggleDislikeSegment("parent");

  // Act & Assert
  EXPECT_FALSE(ShouldFilterSegment("foo"));
}

TEST_F(BraveAdsSegmentUtilTest,
       ShouldFilterAgainstParentForMatchingParentSegmentWithChild) {
  // Arrange
  GetReactions().ToggleDislikeSegment("parent");

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent-child"));
}

TEST_F(BraveAdsSegmentUtilTest,
       ShouldNotFilterAgainstParentForNonMatchingParentSegmentWithChild) {
  // Arrange
  GetReactions().ToggleDislikeSegment("parent");

  // Act & Assert
  EXPECT_FALSE(ShouldFilterSegment("foo-bar"));
}

TEST_F(BraveAdsSegmentUtilTest, HasChildSegment) {
  // Act & Assert
  EXPECT_TRUE(HasChildSegment("technology & computing-windows"));
}

TEST_F(BraveAdsSegmentUtilTest, DoesNotHaveChildSegment) {
  // Act & Assert
  EXPECT_FALSE(HasChildSegment("technology & computing"));
}

}  // namespace brave_ads
