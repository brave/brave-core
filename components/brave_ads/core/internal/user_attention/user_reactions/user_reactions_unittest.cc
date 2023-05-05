/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_reactions/user_reactions.h"

#include <memory>
#include <string>

#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsUserReactionsTest : public AccountObserver, public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    account_ = std::make_unique<Account>(&token_generator_mock_);
    account_->AddObserver(this);

    user_reactions_ = std::make_unique<UserReactions>(*account_);

    MockTokenGenerator(token_generator_mock_, /*count*/ 1);

    privacy::SetUnblindedTokens(/*count*/ 1);
  }

  void TearDown() override {
    account_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  static HistoryItemInfo AddHistoryItem() {
    const CreativeNotificationAdInfo creative_ad =
        BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
    const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

    return HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
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

  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;

  std::unique_ptr<Account> account_;

  bool did_process_deposit_ = false;
  bool failed_to_process_deposit_ = false;

  std::unique_ptr<UserReactions> user_reactions_;
};

TEST_F(BraveAdsUserReactionsTest, LikeAd) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance().LikeAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

TEST_F(BraveAdsUserReactionsTest, DislikeAd) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance().DislikeAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

TEST_F(BraveAdsUserReactionsTest, MarkAdAsInappropriate) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(
      history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

TEST_F(BraveAdsUserReactionsTest, SaveAd) {
  // Arrange
  const HistoryItemInfo history_item = AddHistoryItem();

  // Act
  HistoryManager::GetInstance().ToggleSaveAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
}

}  // namespace brave_ads
