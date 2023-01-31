/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_value_util.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy {

namespace {

constexpr char kJson[] = R"(
  [
    {
      "public_key": "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=",
      "signature": "+yxJmIDobOZ5DBncIVuzjQEZfIa0+UPrSQhzA5pwEAL9qC4UW7A1H35nKAhVLehJlXnnfMVKV02StVO3fBU5CQ==",
      "unblinded_token": "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY"
    },
    {
      "public_key": "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=",
      "signature": "WeBTGGAvueivHOo33UKGTgDRw7fF/Hp9+tNZYDlUjc9CIKt/+ksh4X+mVxSMXc2E1chUWqUDME7DFFuDhasmCg==",
      "unblinded_token": "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K"
    }
  ])";

constexpr char kEmptyJson[] = "[]";

}  // namespace

class BatAdsUnblindedTokenValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsUnblindedTokenValueUtilTest, ToValue) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(2);

  // Act
  const base::Value::List value = UnblindedTokensToValue(unblinded_tokens);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kJson);

  EXPECT_EQ(expected_value, value);
}

TEST_F(BatAdsUnblindedTokenValueUtilTest, ToEmptyValue) {
  // Arrange
  const UnblindedTokenList unblinded_tokens;

  // Act
  const base::Value::List value = UnblindedTokensToValue(unblinded_tokens);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kEmptyJson);

  EXPECT_EQ(expected_value, value);
}

TEST_F(BatAdsUnblindedTokenValueUtilTest, FromValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::List* const list = value.GetIfList();
  ASSERT_TRUE(list);

  // Act
  const UnblindedTokenList unblinded_tokens = UnblindedTokensFromValue(*list);

  // Assert
  const UnblindedTokenList expected_unblinded_tokens = GetUnblindedTokens(2);
  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

TEST_F(BatAdsUnblindedTokenValueUtilTest, FromEmptyValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kEmptyJson);
  const base::Value::List* const list = value.GetIfList();
  ASSERT_TRUE(list);

  // Act
  const UnblindedTokenList unblinded_tokens = UnblindedTokensFromValue(*list);

  // Assert
  const UnblindedTokenList expected_unblinded_tokens;
  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

}  // namespace ads::privacy
