/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmation_util.h"

#include <memory>

#include "bat/ads/internal/account/confirmations/confirmation_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/privacy/tokens/token_generator_mock.h"
#include "bat/ads/internal/privacy/tokens/token_generator_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

constexpr char kTransactionId[] = "8b742869-6e4a-490c-ac31-31b49130098a";
constexpr char kCreativeInstanceId[] = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

}  // namespace

class BatAdsConfirmationUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    token_generator_mock_ =
        std::make_unique<NiceMock<privacy::TokenGeneratorMock>>();
  }

  std::unique_ptr<privacy::TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BatAdsConfirmationUtilTest, CreateConfirmationForNonOptedInUser) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  privacy::SetUnblindedTokens(/*count*/ 1);

  ON_CALL(*token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(1)));

  // Act
  const absl::optional<ConfirmationInfo> confirmation = CreateConfirmation(
      token_generator_mock_.get(), /*created_at*/ Now(), kTransactionId,
      kCreativeInstanceId, ConfirmationType::kViewed, AdType::kNotificationAd,
      base::Value::Dict());
  ASSERT_TRUE(confirmation);

  // Assert
  EXPECT_FALSE(confirmation->opted_in);
  EXPECT_TRUE(IsValid(*confirmation));
}

TEST_F(BatAdsConfirmationUtilTest, IsNotValidForNonOptedInUser) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  // Act
  const ConfirmationInfo confirmation;

  // Assert
  EXPECT_FALSE(IsValid(confirmation));
}

TEST_F(BatAdsConfirmationUtilTest, CreateConfirmationForOptedInUser) {
  // Arrange
  privacy::SetUnblindedTokens(/*count*/ 1);

  ON_CALL(*token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(1)));

  // Act
  const absl::optional<ConfirmationInfo> confirmation = CreateConfirmation(
      token_generator_mock_.get(), /*created_at*/ Now(), kTransactionId,
      kCreativeInstanceId, ConfirmationType::kViewed, AdType::kNotificationAd,
      base::Value::Dict());
  ASSERT_TRUE(confirmation);

  // Assert
  EXPECT_TRUE(confirmation->opted_in);
  EXPECT_TRUE(IsValid(*confirmation));
}

TEST_F(BatAdsConfirmationUtilTest, FailToCreateConfirmationForOptedInUser) {
  // Arrange
  ON_CALL(*token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(1)));

  // Act
  const absl::optional<ConfirmationInfo> confirmation = CreateConfirmation(
      token_generator_mock_.get(), /*created_at*/ Now(), kTransactionId,
      kCreativeInstanceId, ConfirmationType::kViewed, AdType::kNotificationAd,
      base::Value::Dict());

  // Assert
  EXPECT_FALSE(confirmation);
}

TEST_F(BatAdsConfirmationUtilTest, IsNotValidForOptedInUser) {
  // Arrange

  // Act
  const ConfirmationInfo confirmation;

  // Assert
  EXPECT_FALSE(IsValid(confirmation));
}

TEST_F(BatAdsConfirmationUtilTest, ResetConfirmations) {
  // Arrange
  privacy::SetUnblindedTokens(/*count*/ 2);

  privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  ConfirmationStateManager::GetInstance()->AppendFailedConfirmation(
      *confirmation);

  // Act
  ResetConfirmations();

  // Assert
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance()->GetFailedConfirmations();
  EXPECT_TRUE(failed_confirmations.empty());

  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());

  EXPECT_TRUE(privacy::UnblindedTokensIsEmpty());
}

TEST_F(BatAdsConfirmationUtilTest, ResetEmptyConfirmations) {
  // Arrange

  // Act
  ResetConfirmations();

  // Assert
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance()->GetFailedConfirmations();
  EXPECT_TRUE(failed_confirmations.empty());

  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());

  EXPECT_TRUE(privacy::UnblindedTokensIsEmpty());
}

}  // namespace ads
