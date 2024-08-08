/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

#include <optional>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/account_observer_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsReactionsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    GetAccount().AddObserver(&account_observer_mock_);
  }

  void TearDown() override {
    GetAccount().RemoveObserver(&account_observer_mock_);

    test::TestBase::TearDown();
  }

  AccountObserverMock account_observer_mock_;
};

TEST_F(BraveAdsReactionsTest, ToggleLikeAd) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const AdHistoryItemInfo ad_history_item = test::BuildAdHistoryItem(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
                  AdType::kNotificationAd, ConfirmationType::kLikedAd,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().LikeAd(ad_history_item, callback.Get());
  EXPECT_EQ(mojom::ReactionType::kLiked,
            GetReactions().AdReactionTypeForId(test::kAdvertiserId));
}

TEST_F(BraveAdsReactionsTest, ToggleDislikeAd) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const AdHistoryItemInfo ad_history_item = test::BuildAdHistoryItem(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
                  AdType::kNotificationAd, ConfirmationType::kDislikedAd,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().DislikeAd(ad_history_item, callback.Get());
  EXPECT_EQ(mojom::ReactionType::kDisliked,
            GetReactions().AdReactionTypeForId(test::kAdvertiserId));
}

TEST_F(BraveAdsReactionsTest, AdReactionTypeForId) {
  // Arrange
  GetReactions().ToggleLikeAd(test::kAdvertiserId);

  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kLiked,
            GetReactions().AdReactionTypeForId(test::kAdvertiserId));
}

TEST_F(BraveAdsReactionsTest, Ads) {
  // Arrange
  GetReactions().ToggleLikeAd(/*advertiser_id=*/
                              "57b26e2f-2e77-4b31-a3ce-d3faf82dd574");
  GetReactions().ToggleLikeAd(test::kAdvertiserId);
  GetReactions().ToggleDislikeAd(
      /*advertiser_id=*/"2c0577b2-097b-41e8-81db-685de60d26e5");
  GetReactions().ToggleDislikeAd(
      /*advertiser_id=*/"57b26e2f-2e77-4b31-a3ce-d3faf82dd574");

  // Act & Assert
  const ReactionMap expected_ad_reactions = {
      {test::kAdvertiserId, mojom::ReactionType::kLiked},
      {"2c0577b2-097b-41e8-81db-685de60d26e5", mojom::ReactionType::kDisliked},
  };
  EXPECT_EQ(expected_ad_reactions, GetReactions().Ads());
}

TEST_F(BraveAdsReactionsTest, ToggleLikeSegment) {
  // Act
  GetReactions().ToggleLikeSegment(test::kSegment);

  // Assert
  EXPECT_EQ(mojom::ReactionType::kLiked,
            GetReactions().SegmentReactionTypeForId(test::kSegment));
}

TEST_F(BraveAdsReactionsTest, ToggleDislikeSegment) {
  // Act
  GetReactions().ToggleDislikeSegment(test::kSegment);

  // Assert
  EXPECT_EQ(mojom::ReactionType::kDisliked,
            GetReactions().SegmentReactionTypeForId(test::kSegment));
}

TEST_F(BraveAdsReactionsTest, SegmentReactionTypeForId) {
  // Arrange
  GetReactions().ToggleLikeSegment(test::kSegment);

  // Act & Assert
  EXPECT_EQ(mojom::ReactionType::kLiked,
            GetReactions().SegmentReactionTypeForId(test::kSegment));
}

TEST_F(BraveAdsReactionsTest, Segments) {
  // Arrange
  GetReactions().ToggleLikeSegment("technology & computing");
  GetReactions().ToggleLikeSegment(test::kSegment);
  GetReactions().ToggleDislikeSegment("food & drink");
  GetReactions().ToggleDislikeSegment("technology & computing");

  // Act & Assert
  const ReactionMap expected_segment_reactions = {
      {test::kSegment, mojom::ReactionType::kLiked},
      {"food & drink", mojom::ReactionType::kDisliked},
  };
  EXPECT_EQ(expected_segment_reactions, GetReactions().Segments());
}

TEST_F(BraveAdsReactionsTest, ToggleSaveAd) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const AdHistoryItemInfo ad_history_item = test::BuildAdHistoryItem(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
                  AdType::kNotificationAd, ConfirmationType::kSavedAd,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().ToggleSaveAd(ad_history_item, callback.Get());
  EXPECT_TRUE(GetReactions().IsAdSaved(test::kCreativeInstanceId));
}

TEST_F(BraveAdsReactionsTest, IsAdSaved) {
  // Arrange
  GetReactions().ToggleSaveAd(test::kCreativeInstanceId);

  // Act & Assert
  EXPECT_TRUE(GetReactions().IsAdSaved(test::kCreativeInstanceId));
}

TEST_F(BraveAdsReactionsTest, IsAdNotSaved) {
  // Act & Assert
  EXPECT_FALSE(GetReactions().IsAdSaved(test::kCreativeInstanceId));
}

TEST_F(BraveAdsReactionsTest, ToggleMarkAdAsInappropriate) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const AdHistoryItemInfo ad_history_item = test::BuildAdHistoryItem(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          AdType::kNotificationAd, ConfirmationType::kMarkAdAsInappropriate,
          /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().ToggleMarkAdAsInappropriate(ad_history_item,
                                                              callback.Get());
  EXPECT_TRUE(GetReactions().IsAdMarkedAsInappropriate(test::kCreativeSetId));
}

TEST_F(BraveAdsReactionsTest, IsAdMarkedAsInappropriate) {
  // Arrange
  GetReactions().ToggleMarkAdAsInappropriate(test::kCreativeSetId);

  // Act & Assert
  EXPECT_TRUE(GetReactions().IsAdMarkedAsInappropriate(test::kCreativeSetId));
}

TEST_F(BraveAdsReactionsTest, IsAdMarkedAsAppropriate) {
  // Act & Assert
  EXPECT_FALSE(GetReactions().IsAdMarkedAsInappropriate(test::kCreativeSetId));
}

}  // namespace brave_ads
