/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_reactions/user_reactions.h"

#include <memory>
#include <string>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/privacy/tokens/token_generator_mock.h"
#include "bat/ads/internal/privacy/tokens/token_generator_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class BatAdsUserReactionsTest : public AccountObserver, public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    token_generator_mock_ =
        std::make_unique<NiceMock<privacy::TokenGeneratorMock>>();
    account_ = std::make_unique<Account>(token_generator_mock_.get());
    account_->AddObserver(this);

    user_reactions_ = std::make_unique<UserReactions>(account_.get());

    ON_CALL(*token_generator_mock_, Generate(_))
        .WillByDefault(Return(privacy::GetTokens(1)));

    privacy::SetUnblindedTokens(1);
  }

  void TearDown() override {
    account_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  static HistoryItemInfo AddHistoryItem() {
    const CreativeNotificationAdInfo creative_ad =
        BuildCreativeNotificationAd();
    const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

    return HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  }

  void OnDidProcessDeposit(const TransactionInfo& /*transaction*/) override {
    did_process_deposit_ = true;
  }

  void OnFailedToProcessDeposit(
      const std::string& /*creative_instance_id*/,
      const AdType& /*ad_type*/,
      const ConfirmationType& /*confirmation_type*/) override {
    failed_to_process_deposit_ = true;
  }

  std::unique_ptr<privacy::TokenGeneratorMock> token_generator_mock_;
  std::unique_ptr<Account> account_;

  bool did_process_deposit_ = false;
  bool failed_to_process_deposit_ = false;

  std::unique_ptr<UserReactions> user_reactions_;
};

TEST_F(BatAdsUserReactionsTest, LikeAd) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance()->LikeAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

TEST_F(BatAdsUserReactionsTest, DislikeAd) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance()->DislikeAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

TEST_F(BatAdsUserReactionsTest, MarkAdAsInappropriate) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance()->ToggleMarkAdAsInappropriate(
      history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

TEST_F(BatAdsUserReactionsTest, SaveAd) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance()->ToggleSavedAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

}  // namespace ads
