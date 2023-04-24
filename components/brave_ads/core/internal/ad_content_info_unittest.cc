/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ad_content_info.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdContentInfoTest : public UnitTestBase {};

TEST_F(BraveAdsAdContentInfoTest, ToggleFromNeutralToLikeUserReactionType) {
  // Arrange
  const AdContentInfo ad_content;

  // Act

  // Assert
  EXPECT_EQ(mojom::UserReactionType::kLike,
            ad_content.ToggleLikeUserReactionType());
}

TEST_F(BraveAdsAdContentInfoTest, ToggleFromLikeToNeutralUserReactionType) {
  // Arrange
  AdContentInfo ad_content;
  ad_content.user_reaction_type = ad_content.ToggleLikeUserReactionType();
  ASSERT_EQ(mojom::UserReactionType::kLike, ad_content.user_reaction_type);

  // Act

  // Assert
  EXPECT_EQ(mojom::UserReactionType::kNeutral,
            ad_content.ToggleLikeUserReactionType());
}

TEST_F(BraveAdsAdContentInfoTest, ToggleFromNeutralToDislikeUserReactionType) {
  // Arrange
  const AdContentInfo ad_content;

  // Act

  // Assert
  EXPECT_EQ(mojom::UserReactionType::kDislike,
            ad_content.ToggleDislikeUserReactionType());
}

TEST_F(BraveAdsAdContentInfoTest, ToggleFromDisikeToNeutralUserReactionType) {
  // Arrange
  AdContentInfo ad_content;
  ad_content.user_reaction_type = ad_content.ToggleDislikeUserReactionType();
  ASSERT_EQ(mojom::UserReactionType::kDislike, ad_content.user_reaction_type);

  // Act

  // Assert
  EXPECT_EQ(mojom::UserReactionType::kNeutral,
            ad_content.ToggleDislikeUserReactionType());
}

}  // namespace brave_ads
