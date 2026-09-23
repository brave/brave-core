/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/test/payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsPaymentTokensDatabaseTableTest : public test::TestBase {
 protected:
  PaymentTokens database_table_;
};

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, SaveEmptyPaymentTokens) {
  // Act
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({}, save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PaymentTokenList> get_all_test_future;
  database_table_.GetAll(
      get_all_test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, payment_tokens] = get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(payment_tokens, ::testing::IsEmpty());
}

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, SavePaymentTokens) {
  // Arrange
  PaymentTokenInfo payment_token_1 = test::BuildPaymentToken();
  payment_token_1.transaction_id = "foo";
  PaymentTokenInfo payment_token_2 = test::BuildPaymentToken();
  payment_token_2.transaction_id = "bar";

  // Act
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({payment_token_1, payment_token_2},
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PaymentTokenList> get_all_test_future;
  database_table_.GetAll(
      get_all_test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, payment_tokens] = get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((PaymentTokenList{payment_token_1, payment_token_2}),
            payment_tokens);
}

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, SaveDuplicatePaymentTokens) {
  // Arrange
  const PaymentTokenInfo payment_token = test::BuildPaymentToken();
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({payment_token}, save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  base::test::TestFuture<bool> duplicate_test_future;
  database_table_.Save({payment_token}, duplicate_test_future.GetCallback());
  ASSERT_TRUE(duplicate_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PaymentTokenList> get_all_test_future;
  database_table_.GetAll(
      get_all_test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, payment_tokens] = get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(payment_tokens, ::testing::SizeIs(1U));
}

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, DeletePaymentToken) {
  // Arrange
  PaymentTokenInfo payment_token_1 = test::BuildPaymentToken();
  payment_token_1.transaction_id = "foo";
  PaymentTokenInfo payment_token_2 = test::BuildPaymentToken();
  payment_token_2.transaction_id = "bar";
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({payment_token_1, payment_token_2},
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  base::test::TestFuture<bool> delete_test_future;
  database_table_.Delete(payment_token_1, delete_test_future.GetCallback());
  ASSERT_TRUE(delete_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PaymentTokenList> get_all_test_future;
  database_table_.GetAll(
      get_all_test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, payment_tokens] = get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((PaymentTokenList{payment_token_2}), payment_tokens);
}

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, DeletePaymentTokens) {
  // Arrange
  PaymentTokenInfo payment_token_1 = test::BuildPaymentToken();
  payment_token_1.transaction_id = "foo";
  PaymentTokenInfo payment_token_2 = test::BuildPaymentToken();
  payment_token_2.transaction_id = "bar";
  PaymentTokenInfo payment_token_3 = test::BuildPaymentToken();
  payment_token_3.transaction_id = "baz";
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({payment_token_1, payment_token_2, payment_token_3},
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  base::test::TestFuture<bool> delete_test_future;
  database_table_.Delete({payment_token_1, payment_token_2},
                         delete_test_future.GetCallback());
  ASSERT_TRUE(delete_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PaymentTokenList> get_all_test_future;
  database_table_.GetAll(
      get_all_test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, payment_tokens] = get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((PaymentTokenList{payment_token_3}), payment_tokens);
}

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, DeleteEmptyPaymentTokens) {
  // Act & Assert
  base::test::TestFuture<bool> delete_test_future;
  database_table_.Delete(PaymentTokenList{}, delete_test_future.GetCallback());
  EXPECT_TRUE(delete_test_future.Take());
}

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, DoNotDeleteMissingPaymentToken) {
  // Arrange
  PaymentTokenInfo payment_token = test::BuildPaymentToken();
  payment_token.transaction_id = "foo";
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({payment_token}, save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  PaymentTokenInfo missing_payment_token = test::BuildPaymentToken();
  missing_payment_token.transaction_id = "bar";

  // Act
  base::test::TestFuture<bool> delete_test_future;
  database_table_.Delete(missing_payment_token,
                         delete_test_future.GetCallback());
  ASSERT_TRUE(delete_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PaymentTokenList> get_all_test_future;
  database_table_.GetAll(
      get_all_test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, payment_tokens] = get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((PaymentTokenList{payment_token}), payment_tokens);
}

TEST_F(BraveAdsPaymentTokensDatabaseTableTest, DeleteAllPaymentTokens) {
  // Arrange
  PaymentTokenInfo payment_token_1 = test::BuildPaymentToken();
  payment_token_1.transaction_id = "foo";
  PaymentTokenInfo payment_token_2 = test::BuildPaymentToken();
  payment_token_2.transaction_id = "bar";
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({payment_token_1, payment_token_2},
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  base::test::TestFuture<bool> delete_all_test_future;
  database_table_.DeleteAll(delete_all_test_future.GetCallback());
  ASSERT_TRUE(delete_all_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PaymentTokenList> get_all_test_future;
  database_table_.GetAll(
      get_all_test_future.GetCallback<bool, const PaymentTokenList&>());
  const auto [success, payment_tokens] = get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(payment_tokens, ::testing::IsEmpty());
}

}  // namespace brave_ads::database::table
