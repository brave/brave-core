/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kConfirmationTokensAsJson[] =
    R"(
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

}  // namespace

class BraveAdsConfirmationTokenValueUtilTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationTokenValueUtilTest, ConfirmationTokensToValue) {
  // Arrange
  const ConfirmationTokenList confirmation_tokens =
      test::BuildConfirmationTokens(/*count=*/2);

  // Act
  const base::Value::List list = ConfirmationTokensToValue(confirmation_tokens);

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kConfirmationTokensAsJson), list);
}

TEST_F(BraveAdsConfirmationTokenValueUtilTest, EmptyConfirmationTokensToValue) {
  // Act
  const base::Value::List list = ConfirmationTokensToValue({});

  // Assert
  EXPECT_THAT(list, ::testing::IsEmpty());
}

TEST_F(BraveAdsConfirmationTokenValueUtilTest, ConfirmationTokensFromValue) {
  // Arrange
  const base::Value::List list =
      base::test::ParseJsonList(kConfirmationTokensAsJson);

  // Act
  const ConfirmationTokenList confirmation_tokens =
      ConfirmationTokensFromValue(list);

  // Assert
  EXPECT_EQ(test::BuildConfirmationTokens(/*count=*/2), confirmation_tokens);
}

TEST_F(BraveAdsConfirmationTokenValueUtilTest,
       EmptyConfirmationTokensFromValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList("[]");

  // Act
  const ConfirmationTokenList confirmation_tokens =
      ConfirmationTokensFromValue(list);

  // Assert
  EXPECT_THAT(confirmation_tokens, ::testing::IsEmpty());
}

}  // namespace brave_ads
