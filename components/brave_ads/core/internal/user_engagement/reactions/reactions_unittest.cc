/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

#include <memory>
#include <optional>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/account_observer_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsReactionsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

    account_ = std::make_unique<Account>(&token_generator_mock_);
    account_->AddObserver(&account_observer_mock_);

    reactions_ = std::make_unique<Reactions>(*account_);

    test::ForcePermissionRules();
  }

  void TearDown() override {
    account_->RemoveObserver(&account_observer_mock_);

    test::TestBase::TearDown();
  }

  ::testing::NiceMock<TokenGeneratorMock> token_generator_mock_;

  std::unique_ptr<Account> account_;
  AccountObserverMock account_observer_mock_;

  std::unique_ptr<Reactions> reactions_;
};

TEST_F(BraveAdsReactionsTest, LikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  const std::optional<AdHistoryItemInfo> ad_history_item =
      AdHistoryManager::GetInstance().Add(ad,
                                          ConfirmationType::kViewedImpression);
  ASSERT_TRUE(ad_history_item);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
                  AdType::kNotificationAd, ConfirmationType::kLikedAd,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleUserReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().LikeAd(*ad_history_item, callback.Get());
}

TEST_F(BraveAdsReactionsTest, DislikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  const std::optional<AdHistoryItemInfo> ad_history_item =
      AdHistoryManager::GetInstance().Add(ad,
                                          ConfirmationType::kViewedImpression);
  ASSERT_TRUE(ad_history_item);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
                  AdType::kNotificationAd, ConfirmationType::kDislikedAd,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleUserReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().DislikeAd(*ad_history_item, callback.Get());
}

TEST_F(BraveAdsReactionsTest, MarkAdAsInappropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  const std::optional<AdHistoryItemInfo> ad_history_item =
      AdHistoryManager::GetInstance().Add(ad,
                                          ConfirmationType::kViewedImpression);
  ASSERT_TRUE(ad_history_item);

  // Act & Assert
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          AdType::kNotificationAd, ConfirmationType::kMarkAdAsInappropriate,
          /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleUserReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().ToggleMarkAdAsInappropriate(*ad_history_item,
                                                              callback.Get());
}

TEST_F(BraveAdsReactionsTest, SaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  const std::optional<AdHistoryItemInfo> ad_history_item =
      AdHistoryManager::GetInstance().Add(ad,
                                          ConfirmationType::kViewedImpression);
  ASSERT_TRUE(ad_history_item);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
                  AdType::kNotificationAd, ConfirmationType::kSavedAd,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);

  base::MockCallback<ToggleUserReactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  AdHistoryManager::GetInstance().ToggleSaveAd(*ad_history_item,
                                               callback.Get());
}

}  // namespace brave_ads
