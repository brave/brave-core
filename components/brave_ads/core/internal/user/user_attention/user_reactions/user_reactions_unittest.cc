/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_attention/user_reactions/user_reactions.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/account_observer_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/history_item_util.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/ad_content_info.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserReactionsTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

    account_ = std::make_unique<Account>(&token_generator_mock_);
    account_->AddObserver(&observer_mock_);

    user_reactions_ = std::make_unique<UserReactions>(*account_);

    test::ForcePermissionRules();
  }

  void TearDown() override {
    account_->RemoveObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  ::testing::NiceMock<TokenGeneratorMock> token_generator_mock_;

  std::unique_ptr<Account> account_;
  AccountObserverMock observer_mock_;

  std::unique_ptr<UserReactions> user_reactions_;
};

TEST_F(BraveAdsUserReactionsTest, LikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);
  const AdContentInfo& ad_content = history_item.ad_content;

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(observer_mock_, OnFailedToProcessDeposit).Times(0);
  HistoryManager::GetInstance().LikeAd(ad_content);
}

TEST_F(BraveAdsUserReactionsTest, DislikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);
  const AdContentInfo& ad_content = history_item.ad_content;

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(observer_mock_, OnFailedToProcessDeposit).Times(0);
  HistoryManager::GetInstance().DislikeAd(ad_content);
}

TEST_F(BraveAdsUserReactionsTest, MarkAdAsInappropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);
  const AdContentInfo& ad_content = history_item.ad_content;

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(observer_mock_, OnFailedToProcessDeposit).Times(0);
  HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(ad_content);
}

TEST_F(BraveAdsUserReactionsTest, SaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);
  const AdContentInfo& ad_content = history_item.ad_content;

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(observer_mock_, OnFailedToProcessDeposit).Times(0);
  HistoryManager::GetInstance().ToggleSaveAd(ad_content);
}

}  // namespace brave_ads
