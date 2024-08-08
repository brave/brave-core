/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_type_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsReactionsTypeUtilTest : public test::TestBase {};

TEST_F(BraveAdsReactionsTypeUtilTest,
       ToggleLikedReactionTypeFromNeutralToLiked) {
  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kLiked,
            ToggleLikedReactionType(mojom::ReactionType::kNeutral));
}

TEST_F(BraveAdsReactionsTypeUtilTest,
       ToggleLikedReactionTypeFromLikedToNeutral) {
  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kNeutral,
            ToggleLikedReactionType(mojom::ReactionType::kLiked));
}

TEST_F(BraveAdsReactionsTypeUtilTest,
       ToggleLikedReactionTypeFromDislikedToNeutral) {
  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kNeutral,
            ToggleLikedReactionType(mojom::ReactionType::kDisliked));
}

TEST_F(BraveAdsReactionsTypeUtilTest,
       ToggleDislikedReactionTypeFromNeutralToDisliked) {
  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kDisliked,
            ToggleDislikedReactionType(mojom::ReactionType::kNeutral));
}

TEST_F(BraveAdsReactionsTypeUtilTest,
       ToggleDislikedReactionTypeFromDislikedToNeutral) {
  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kNeutral,
            ToggleDislikedReactionType(mojom::ReactionType::kDisliked));
}

TEST_F(BraveAdsReactionsTypeUtilTest,
       ToggleDislikedReactionTypeFromLikedToNeutral) {
  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kNeutral,
            ToggleDislikedReactionType(mojom::ReactionType::kLiked));
}

}  // namespace brave_ads
