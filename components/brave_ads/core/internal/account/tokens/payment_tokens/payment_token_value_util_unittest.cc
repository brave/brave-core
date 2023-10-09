/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kJson[] =
    R"(
        [
          {
            "ad_type": "ad_notification",
            "confirmation_type": "view",
            "public_key": "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=",
            "transaction_id": "0d9de7ce-b3f9-4158-8726-23d52b9457c6",
            "unblinded_token": "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY"
          },
          {
            "ad_type": "ad_notification",
            "confirmation_type": "view",
            "public_key": "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=",
            "transaction_id": "0d9de7ce-b3f9-4158-8726-23d52b9457c6",
            "unblinded_token": "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K"
          }
        ])";

constexpr char kEmptyJson[] = "[]";

}  // namespace

class BraveAdsPaymentTokenValueUtilTest : public UnitTestBase {};

TEST_F(BraveAdsPaymentTokenValueUtilTest, ToValue) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonList(kJson),
            PaymentTokensToValue(test::BuildPaymentTokens(/*count=*/2)));
}

TEST_F(BraveAdsPaymentTokenValueUtilTest, ToEmptyValue) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonList(kEmptyJson), PaymentTokensToValue({}));
}

TEST_F(BraveAdsPaymentTokenValueUtilTest, FromValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList(kJson);

  // Act & Assert
  EXPECT_EQ(test::BuildPaymentTokens(/*count=*/2),
            PaymentTokensFromValue(list));
}

TEST_F(BraveAdsPaymentTokenValueUtilTest, FromEmptyValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList(kEmptyJson);

  // Act
  const PaymentTokenList payment_tokens = PaymentTokensFromValue(list);

  // Assert
  EXPECT_TRUE(payment_tokens.empty());
}

}  // namespace brave_ads
