/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ad_content_info.h"

#include "brave/components/brave_ads/core/ad_content_action_types.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsAdContentInfoTest : public UnitTestBase {};

TEST_F(BatAdsAdContentInfoTest, ToggleThumbUp) {
  // Arrange
  const AdContentInfo ad_content;

  // Act
  const AdContentLikeActionType action_type =
      ad_content.ToggleThumbUpActionType();

  // Assert
  EXPECT_EQ(AdContentLikeActionType::kThumbsUp, action_type);
}

TEST_F(BatAdsAdContentInfoTest, ToggleThumbUpToNetrual) {
  // Arrange
  AdContentInfo ad_content;
  ad_content.like_action_type = ad_content.ToggleThumbUpActionType();
  ASSERT_EQ(AdContentLikeActionType::kThumbsUp, ad_content.like_action_type);

  // Act
  const AdContentLikeActionType like_action_type =
      ad_content.ToggleThumbUpActionType();

  // Assert
  EXPECT_EQ(AdContentLikeActionType::kNeutral, like_action_type);
}

TEST_F(BatAdsAdContentInfoTest, ToggleThumbDown) {
  // Arrange
  const AdContentInfo ad_content;

  // Act
  const AdContentLikeActionType like_action_type =
      ad_content.ToggleThumbDownActionType();

  // Assert
  EXPECT_EQ(AdContentLikeActionType::kThumbsDown, like_action_type);
}

TEST_F(BatAdsAdContentInfoTest, ToggleThumbDownToNetrual) {
  // Arrange
  AdContentInfo ad_content;
  ad_content.like_action_type = ad_content.ToggleThumbDownActionType();
  ASSERT_EQ(AdContentLikeActionType::kThumbsDown, ad_content.like_action_type);

  // Act
  const AdContentLikeActionType like_action_type =
      ad_content.ToggleThumbDownActionType();

  // Assert
  EXPECT_EQ(AdContentLikeActionType::kNeutral, like_action_type);
}

}  // namespace brave_ads
