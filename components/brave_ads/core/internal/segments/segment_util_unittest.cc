/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_json_reader.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/history/category_content_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kCatalog[] = "catalog_with_multiple_campaigns.json";
}  // namespace

class BraveAdsSegmentUtilTest : public UnitTestBase {};

TEST_F(BraveAdsSegmentUtilTest, GetSegmentsFromCatalog) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(json);

  const absl::optional<CatalogInfo> catalog = json::reader::ReadCatalog(*json);
  ASSERT_TRUE(catalog);

  // Act & Assert
  const SegmentList expected_segments = {"technology & computing",
                                         "untargeted"};
  EXPECT_EQ(expected_segments, GetSegments(*catalog));
}

TEST_F(BraveAdsSegmentUtilTest, GetSegmentsFromEmptyCatalog) {
  // Act
  const SegmentList segments = GetSegments(/*catalog=*/{});

  // Assert
  EXPECT_TRUE(segments.empty());
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

  // Act & Assert
  const SegmentList expected_parent_segments = {
      "technology & computing", "personal finance", "automotive"};
  EXPECT_EQ(expected_parent_segments, GetParentSegments(segments));
}

TEST_F(BraveAdsSegmentUtilTest, GetParentSegmentsForEmptyList) {
  // Act
  const SegmentList parent_segments = GetParentSegments(/*segments=*/{});

  // Assert
  EXPECT_TRUE(parent_segments.empty());
}

TEST_F(BraveAdsSegmentUtilTest, ShouldFilterMatchingParentChildSegment) {
  // Arrange
  CategoryContentInfo category_content;
  category_content.category = "parent-child";
  category_content.user_reaction_type = mojom::UserReactionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeCategory(category_content);

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent-child"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentChildSegment) {
  // Arrange
  CategoryContentInfo category_content;
  category_content.category = "parent-child";
  category_content.user_reaction_type = mojom::UserReactionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeCategory(category_content);

  // Act & Assert
  EXPECT_FALSE(ShouldFilterSegment("foo-bar"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldFilterMatchingParentSegment) {
  // Arrange
  CategoryContentInfo category_content;
  category_content.category = "parent";
  category_content.user_reaction_type = mojom::UserReactionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeCategory(category_content);

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent"));
}

TEST_F(BraveAdsSegmentUtilTest, ShouldNotFilterNonMatchingParentSegment) {
  // Arrange
  CategoryContentInfo category_content;
  category_content.category = "parent";
  category_content.user_reaction_type = mojom::UserReactionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeCategory(category_content);

  // Act & Assert
  EXPECT_FALSE(ShouldFilterSegment("foo"));
}

TEST_F(BraveAdsSegmentUtilTest,
       ShouldFilterAgainstParentForMatchingParentSegmentWithChild) {
  // Arrange
  CategoryContentInfo category_content;
  category_content.category = "parent";
  category_content.user_reaction_type = mojom::UserReactionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeCategory(category_content);

  // Act & Assert
  EXPECT_TRUE(ShouldFilterSegment("parent-child"));
}

TEST_F(BraveAdsSegmentUtilTest,
       ShouldNotFilterAgainstParentForNonMatchingParentSegmentWithChild) {
  // Arrange
  CategoryContentInfo category_content;
  category_content.category = "parent";
  category_content.user_reaction_type = mojom::UserReactionType::kNeutral;
  ClientStateManager::GetInstance().ToggleDislikeCategory(category_content);

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
