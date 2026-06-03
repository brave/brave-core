/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"

#include <cstddef>

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/wallet/test/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kConfirmationsJsonFilename[] = "confirmations.json";
constexpr char kConfirmationsWithMultipleTokensJsonFilename[] =
    "confirmations_with_multiple_tokens.json";
constexpr char kConfirmationsWithSingleTokenJsonFilename[] =
    "confirmations_with_single_token.json";
constexpr char kConfirmationsWithNoTokensJsonFilename[] =
    "confirmations_with_no_tokens.json";
constexpr char kConfirmationsWithCorruptConfirmationTokensJsonFilename[] =
    "confirmations_with_corrupt_confirmation_tokens.json";
constexpr char kConfirmationsWithCorruptPaymentTokensJsonFilename[] =
    "confirmations_with_corrupt_payment_tokens.json";

size_t GetConfirmationTokenCount() {
  base::test::TestFuture<bool, ConfirmationTokenList> test_future;
  database::table::ConfirmationTokens().GetAll(
      test_future.GetCallback<bool, const ConfirmationTokenList&>());
  const auto [success, tokens] = test_future.Take();
  EXPECT_TRUE(success);
  return tokens.size();
}

size_t GetPaymentTokenCount() {
  base::test::TestFuture<bool, PaymentTokenList> test_future;
  database::table::PaymentTokens().GetAll(
      test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, tokens] = test_future.Take();
  EXPECT_TRUE(success);
  return tokens.size();
}

}  // namespace

class BraveAdsLegacyConfirmationMigrationTest : public test::TestBase {};

TEST_F(BraveAdsLegacyConfirmationMigrationTest, MigrateWithMultipleTokens) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kConfirmationsWithMultipleTokensJsonFilename,
      kConfirmationsJsonFilename));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(test::Wallet(), test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(2U, GetConfirmationTokenCount());
  EXPECT_EQ(2U, GetPaymentTokenCount());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest, MigrateWithSingleToken) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kConfirmationsWithSingleTokenJsonFilename, kConfirmationsJsonFilename));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(test::Wallet(), test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(1U, GetConfirmationTokenCount());
  EXPECT_EQ(1U, GetPaymentTokenCount());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest,
       MigrateWithEmptyConfirmationState) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kConfirmationsWithNoTokensJsonFilename, kConfirmationsJsonFilename));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(test::Wallet(), test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(0U, GetConfirmationTokenCount());
  EXPECT_EQ(0U, GetPaymentTokenCount());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest,
       MigrateAndDropCorruptConfirmationTokenRows) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kConfirmationsWithCorruptConfirmationTokensJsonFilename,
      kConfirmationsJsonFilename));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(test::Wallet(), test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(1U, GetConfirmationTokenCount());
  EXPECT_EQ(0U, GetPaymentTokenCount());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest,
       MigrateAndDropCorruptPaymentTokenRows) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kConfirmationsWithCorruptPaymentTokensJsonFilename,
      kConfirmationsJsonFilename));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(test::Wallet(), test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(0U, GetConfirmationTokenCount());
  EXPECT_EQ(1U, GetPaymentTokenCount());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest,
       MigrateWhenConfirmationStateIsMalformed) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      test::kMalformedJsonFilename, kConfirmationsJsonFilename));

  // Act & Assert
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(test::Wallet(), test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest,
       MigrateWhenConfirmationStateDoesNotExist) {
  // Act
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(test::Wallet(), test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(0U, GetConfirmationTokenCount());
  EXPECT_EQ(0U, GetPaymentTokenCount());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest, DoNotMigrateWithoutWallet) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kConfirmationsWithMultipleTokensJsonFilename,
      kConfirmationsJsonFilename));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateConfirmationState(/*wallet=*/std::nullopt, test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(0U, GetConfirmationTokenCount());
  EXPECT_EQ(0U, GetPaymentTokenCount());
}

}  // namespace brave_ads
