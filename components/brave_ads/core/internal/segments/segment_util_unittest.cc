/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

#include <optional>
#include <utility>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/tokens/test/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_json_reader.h"
#include "brave/components/brave_ads/core/internal/catalog/test/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/file_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/test/reactions_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void DislikeSegment(const std::string& segment) {
  mojom::ReactionInfoPtr mojom_reaction =
      test::BuildReaction(mojom::AdType::kNotificationAd);
  mojom_reaction->segment = segment;

  test::MockTokenGenerator(/*count=*/1);
  base::MockCallback<ResultCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleDislikeSegment(std::move(mojom_reaction),
                                      callback.Get());
}

}  // namespace

class BraveAdsSegmentUtilTest : public test::TestBase {};

TEST_F(BraveAdsSegmentUtilTest, GetSegmentsFromCatalog) {
  // Arrange
  std::optional<std::string> contents =
      test::MaybeReadFileToStringAndReplaceTags(
          test::kCatalogWithMultipleCampaignsJsonFilename);
  ASSERT_TRUE(contents);

  std::optional<CatalogInfo> catalog = json::reader::ReadCatalog(*contents);
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
  DislikeSegment("parent-child");

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent-child"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentChildSegment) {
  // Arrange
  DislikeSegment("parent-child");

  // Act & Assert
  EXPECT_FALSE(ShouldFilterSegment("foo-bar"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldFilterMatchingParentSegment) {
  // Arrange
  DislikeSegment("parent");

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentSegment) {
  // Arrange
  DislikeSegment("parent");

  // Act & Assert
  EXPECT_FALSE(ShouldFilterSegment("foo"));
}

TEST_F(BraveAdsSegmentUtilTest,
       ShouldFilterAgainstParentForMatchingParentSegmentWithChild) {
  // Arrange
  DislikeSegment("parent");

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent-child"));
}

TEST_F(BraveAdsSegmentUtilTest,
       ShouldNotFilterAgainstParentForNonMatchingParentSegmentWithChild) {
  // Arrange
  DislikeSegment("parent");

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
