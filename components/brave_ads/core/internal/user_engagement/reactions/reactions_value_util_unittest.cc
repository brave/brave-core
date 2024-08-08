/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kReactionMapAsJson[] =
    R"(
        {
          "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2": 1,
          "untargeted": -1
        })";

constexpr char kReactionSetAsJson[] =
    R"(
        [
          "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
          "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123"
        ])";

}  // namespace

TEST(BraveAdsReactionsValueUtilTest, ReactionMapToDict) {
  // Arrange
  const ReactionMap reactions = {
      {test::kAdvertiserId, mojom::ReactionType::kLiked},
      {test::kSegment, mojom::ReactionType::kDisliked}};

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(kReactionMapAsJson),
            ReactionMapToDict(reactions));
}

TEST(BraveAdsReactionsValueUtilTest, EmptyReactionMapToDict) {
  // Act & Assert
  EXPECT_THAT(ReactionMapToDict({}), ::testing::IsEmpty());
}

TEST(BraveAdsReactionsValueUtilTest, ReactionMapFromDict) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(kReactionMapAsJson);

  // Act & Assert
  const ReactionMap expected_reactions = {
      {test::kAdvertiserId, mojom::ReactionType::kLiked},
      {test::kSegment, mojom::ReactionType::kDisliked}};
  EXPECT_EQ(expected_reactions, ReactionMapFromDict(dict));
}

TEST(BraveAdsReactionsValueUtilTest, ReactionMapFromEmptyDict) {
  // Act & Assert
  EXPECT_THAT(ReactionMapFromDict({}), ::testing::IsEmpty());
}

TEST(BraveAdsReactionsValueUtilTest, ReactionSetToList) {
  // Arrange
  const ReactionSet reactions = {test::kCreativeInstanceId,
                                 test::kCreativeSetId};

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonList(kReactionSetAsJson),
            ReactionSetToList(reactions));
}

TEST(BraveAdsReactionsValueUtilTest, EmptyReactionSetToList) {
  // Act & Assert
  EXPECT_THAT(ReactionSetToList({}), ::testing::IsEmpty());
}

TEST(BraveAdsReactionsValueUtilTest, ReactionSetFromList) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList(kReactionSetAsJson);

  // Act & Assert
  const ReactionSet expected_reactions = {test::kCreativeInstanceId,
                                          test::kCreativeSetId};
  EXPECT_EQ(expected_reactions, ReactionSetFromList(list));
}

TEST(BraveAdsReactionsValueUtilTest, ReactionSetFromEmptyList) {
  // Act & Assert
  EXPECT_THAT(ReactionSetFromList({}), ::testing::IsEmpty());
}

}  // namespace brave_ads
