/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

#include <optional>
#include <utility>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/account_observer_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"

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

  mojom::ReactionInfoPtr mojom_reaction =
      test::BuildReaction(mojom::AdType::kNotificationAd);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          mojom::AdType::kNotificationAd, mojom::ConfirmationType::kLikedAd,
          /*reconciled_at*/ std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleLikeAd(std::move(mojom_reaction), callback.Get());
  run_loop.Run();
  EXPECT_EQ(mojom::ReactionType::kLiked,
            GetReactions().AdReactionTypeForId(test::kAdvertiserId));
}

TEST_F(BraveAdsReactionsTest, ToggleDislikeAd) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  mojom::ReactionInfoPtr mojom_reaction =
      test::BuildReaction(mojom::AdType::kNotificationAd);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          mojom::AdType::kNotificationAd, mojom::ConfirmationType::kDislikedAd,
          /*reconciled_at*/ std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleDislikeAd(std::move(mojom_reaction), callback.Get());
  run_loop.Run();
  EXPECT_EQ(mojom::ReactionType::kDisliked,
            GetReactions().AdReactionTypeForId(test::kAdvertiserId));
}

TEST_F(BraveAdsReactionsTest, Ads) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->advertiser_id = test::kAnotherCampaignId;

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleLikeAd(std::move(mojom_reaction), callback.Get());
  }

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->advertiser_id = test::kAdvertiserId;

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleLikeAd(std::move(mojom_reaction), callback.Get());
  }

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->advertiser_id = "2c0577b2-097b-41e8-81db-685de60d26e5";

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleDislikeAd(std::move(mojom_reaction), callback.Get());
  }

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->advertiser_id = test::kAnotherCampaignId;

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleDislikeAd(std::move(mojom_reaction), callback.Get());
  }

  // Act & Assert
  const ReactionMap expected_ad_reactions = {
      {test::kAdvertiserId, mojom::ReactionType::kLiked},
      {/*advertiser_id=*/"2c0577b2-097b-41e8-81db-685de60d26e5",
       mojom::ReactionType::kDisliked},
      {test::kAnotherCampaignId, mojom::ReactionType::kDisliked}};
  EXPECT_EQ(expected_ad_reactions, GetReactions().Ads());
}

TEST_F(BraveAdsReactionsTest, ToggleLikeSegment) {
  // Act
  mojom::ReactionInfoPtr mojom_reaction =
      test::BuildReaction(mojom::AdType::kNotificationAd);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleLikeSegment(std::move(mojom_reaction), callback.Get());

  // Assert
  EXPECT_EQ(mojom::ReactionType::kLiked,
            GetReactions().SegmentReactionTypeForId(test::kSegment));
}

TEST_F(BraveAdsReactionsTest, ToggleDislikeSegment) {
  // Act
  mojom::ReactionInfoPtr mojom_reaction =
      test::BuildReaction(mojom::AdType::kNotificationAd);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleDislikeSegment(std::move(mojom_reaction),
                                      callback.Get());

  // Assert
  EXPECT_EQ(mojom::ReactionType::kDisliked,
            GetReactions().SegmentReactionTypeForId(test::kSegment));
}

TEST_F(BraveAdsReactionsTest, Segments) {
  // Arrange

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->segment = "technology & computing";

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleLikeSegment(std::move(mojom_reaction), callback.Get());
  }

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->segment = test::kSegment;

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleLikeSegment(std::move(mojom_reaction), callback.Get());
  }

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->segment = "food & drink";

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleDislikeSegment(std::move(mojom_reaction),
                                        callback.Get());
  }

  {
    mojom::ReactionInfoPtr mojom_reaction =
        test::BuildReaction(mojom::AdType::kNotificationAd);
    mojom_reaction->segment = "technology & computing";

    base::MockCallback<ToggleReactionCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/true));
    GetReactions().ToggleDislikeSegment(std::move(mojom_reaction),
                                        callback.Get());
  }

  // Act & Assert
  const ReactionMap expected_segment_reactions = {
      {test::kSegment, mojom::ReactionType::kLiked},
      {"technology & computing", mojom::ReactionType::kDisliked},
      {"food & drink", mojom::ReactionType::kDisliked}};
  EXPECT_EQ(expected_segment_reactions, GetReactions().Segments());
}

TEST_F(BraveAdsReactionsTest, ToggleSaveAd) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  mojom::ReactionInfoPtr mojom_reaction =
      test::BuildReaction(mojom::AdType::kNotificationAd);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          mojom::AdType::kNotificationAd, mojom::ConfirmationType::kSavedAd,
          /*reconciled_at*/ std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleSaveAd(std::move(mojom_reaction), callback.Get());
  run_loop.Run();
  EXPECT_TRUE(GetReactions().IsAdSaved(test::kCreativeInstanceId));
}

TEST_F(BraveAdsReactionsTest, IsAdNotSaved) {
  // Act & Assert
  EXPECT_FALSE(GetReactions().IsAdSaved(test::kCreativeInstanceId));
}

TEST_F(BraveAdsReactionsTest, ToggleMarkAdAsInappropriate) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  mojom::ReactionInfoPtr mojom_reaction =
      test::BuildReaction(mojom::AdType::kNotificationAd);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
                  mojom::AdType::kNotificationAd,
                  mojom::ConfirmationType::kMarkAdAsInappropriate,
                  /*reconciled_at*/ std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  GetReactions().ToggleMarkAdAsInappropriate(std::move(mojom_reaction),
                                             callback.Get());
  run_loop.Run();
  EXPECT_TRUE(GetReactions().IsAdMarkedAsInappropriate(test::kCreativeSetId));
}

TEST_F(BraveAdsReactionsTest, IsAdMarkedAsAppropriate) {
  // Act & Assert
  EXPECT_FALSE(GetReactions().IsAdMarkedAsInappropriate(test::kCreativeSetId));
}

}  // namespace brave_ads
