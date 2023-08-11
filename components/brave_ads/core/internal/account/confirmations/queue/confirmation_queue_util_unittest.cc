/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_util.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/catalog_permission_rule_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsConversionQueueUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    auto& sys_info = GlobalState::GetInstance()->SysInfo();
    sys_info.device_id =
        "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

    ForceCatalogPermissionRuleForTesting();

    SetDefaultStringPref(prefs::kDiagnosticId,
                         "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

    AdvanceClockTo(
        TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));
  }

  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BraveAdsConversionQueueUtilTest, AddRewardConfirmationQueueItem) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act
  AddConfirmationQueueItem(*confirmation);

  // Assert
  EXPECT_EQ(confirmation, MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest,
       AddConversionRewardConfirmationQueueItem) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      /*verifiable_conversion*/ absl::nullopt);
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act
  AddConfirmationQueueItem(*confirmation);

  // Assert
  EXPECT_EQ(confirmation, MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest,
       AddVerifiableConversionRewardConfirmationQueueItem) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act
  AddConfirmationQueueItem(*confirmation);

  // Assert
  EXPECT_EQ(confirmation, MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest, AddNonRewardConfirmationQueueItem) {
  // Arrange
  DisableBraveRewardsForTesting();

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act
  AddConfirmationQueueItem(*confirmation);

  // Assert
  EXPECT_EQ(confirmation, MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest,
       AddConversionNonRewardConfirmationQueueItem) {
  // Arrange
  DisableBraveRewardsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt);
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act
  AddConfirmationQueueItem(*confirmation);

  // Assert
  EXPECT_EQ(confirmation, MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest,
       AddVerifiableConversionNonRewardConfirmationQueueItem) {
  // Arrange
  DisableBraveRewardsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kClicked,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act
  AddConfirmationQueueItem(*confirmation);

  // Assert
  EXPECT_EQ(confirmation, MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest, RemoveConfirmationQueueItem) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  AddConfirmationQueueItem(*confirmation);
  ASSERT_TRUE(MaybeGetNextConfirmationQueueItem());

  // Act
  RemoveConfirmationQueueItem(*confirmation);

  // Assert
  EXPECT_FALSE(MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest, GetRewardConfirmationQueueItem) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  AddConfirmationQueueItem(*confirmation);

  // Act

  // Assert
  EXPECT_TRUE(MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest, GetNonRewardConfirmationQueueItem) {
  // Arrange
  DisableBraveRewardsForTesting();

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction,
                                 /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  AddConfirmationQueueItem(*confirmation);

  // Act

  // Assert
  EXPECT_TRUE(MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest,
       DoNotGetConfirmationQueueItemIfQueueIsEmpty) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(MaybeGetNextConfirmationQueueItem());
}

TEST_F(BraveAdsConversionQueueUtilTest, RebuildRewardConfirmationQueueItem) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  base::MockCallback<RebuildConfirmationQueueItemCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&confirmation](const ConfirmationInfo& rebuilt_confirmation) {
        // Assert
        EXPECT_NE(confirmation, rebuilt_confirmation);
      });

  // Act
  RebuildConfirmationQueueItem(*confirmation, callback.Get());
}

TEST_F(BraveAdsConversionQueueUtilTest, RebuildNonRewardConfirmationQueueItem) {
  // Arrange
  DisableBraveRewardsForTesting();

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  base::MockCallback<RebuildConfirmationQueueItemCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&confirmation](const ConfirmationInfo& rebuilt_confirmation) {
        // Assert
        EXPECT_EQ(confirmation, rebuilt_confirmation);
      });

  // Act
  RebuildConfirmationQueueItem(*confirmation, callback.Get());
}

}  // namespace brave_ads
