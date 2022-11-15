/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy {

class BatAdsUnblindedPaymentTokenUtilTest : public UnitTestBase {};

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, GetUnblindedPaymentTokens) {
  // Arrange
  BuildAndSetUnblindedPaymentTokens(/*count*/ 10);

  // Act
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      GetUnblindedPaymentTokens(/*count*/ 5);

  // Assert
  EXPECT_EQ(5U, unblinded_payment_tokens.size());
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, GetAllUnblindedPaymentTokens) {
  // Arrange
  BuildAndSetUnblindedPaymentTokens(/*count*/ 10);

  // Act
  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      GetAllUnblindedPaymentTokens();

  // Assert
  EXPECT_EQ(10U, unblinded_payment_tokens.size());
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, AddUnblindedPaymentTokens) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildUnblindedPaymentTokens(/*count*/ 3);
  ASSERT_EQ(3U, unblinded_payment_tokens.size());

  const UnblindedPaymentTokenInfo& token_1 = unblinded_payment_tokens.at(0);
  const UnblindedPaymentTokenInfo& token_2 = unblinded_payment_tokens.at(1);
  const UnblindedPaymentTokenInfo& token_3 = unblinded_payment_tokens.at(2);

  SetUnblindedPaymentTokens({token_1});

  // Act
  AddUnblindedPaymentTokens({token_2, token_3});

  // Assert
  const UnblindedPaymentTokenList expected_tokens = {token_1, token_2, token_3};
  EXPECT_EQ(expected_tokens, GetAllUnblindedPaymentTokens());
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, RemoveUnblindedPaymentTokens) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildAndSetUnblindedPaymentTokens(/*count*/ 3);
  ASSERT_EQ(3U, unblinded_payment_tokens.size());

  const UnblindedPaymentTokenInfo& token_1 = unblinded_payment_tokens.at(0);
  const UnblindedPaymentTokenInfo& token_2 = unblinded_payment_tokens.at(1);
  const UnblindedPaymentTokenInfo& token_3 = unblinded_payment_tokens.at(2);

  // Act
  RemoveUnblindedPaymentTokens({token_1, token_3});

  // Assert
  const UnblindedPaymentTokenList expected_tokens = {token_2};
  EXPECT_EQ(expected_tokens, GetAllUnblindedPaymentTokens());
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, RemoveAllUnblindedPaymentTokens) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildAndSetUnblindedPaymentTokens(/*count*/ 2);

  // Act
  RemoveAllUnblindedPaymentTokens();

  // Assert
  EXPECT_TRUE(UnblindedPaymentTokensIsEmpty());
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, UnblindedPaymentTokenExists) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildAndSetUnblindedPaymentTokens(/*count*/ 1);
  ASSERT_EQ(1U, unblinded_payment_tokens.size());

  // Act

  // Assert
  const UnblindedPaymentTokenInfo unblinded_payment_token =
      BuildUnblindedPaymentToken(
          R"(PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY)");

  EXPECT_TRUE(UnblindedPaymentTokenExists(unblinded_payment_token));
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, UnblindedPaymentTokenDoesNotExist) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildAndSetUnblindedPaymentTokens(/*count*/ 1);

  // Act

  // Assert
  const UnblindedPaymentTokenInfo unblinded_payment_token =
      BuildUnblindedPaymentToken(
          R"(hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K)");

  EXPECT_FALSE(UnblindedPaymentTokenExists(unblinded_payment_token));
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, UnblindedPaymentTokensIsEmpty) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UnblindedPaymentTokensIsEmpty());
}

TEST_F(BatAdsUnblindedPaymentTokenUtilTest, UnblindedPaymentTokenCount) {
  // Arrange
  BuildAndSetUnblindedPaymentTokens(/*count*/ 2);

  // Act

  // Assert
  EXPECT_EQ(2, UnblindedPaymentTokenCount());
}

}  // namespace ads::privacy
