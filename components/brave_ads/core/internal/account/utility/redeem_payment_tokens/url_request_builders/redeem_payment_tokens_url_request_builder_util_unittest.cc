/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/url_request_builders/redeem_payment_tokens_url_request_builder_util.h"

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsRedeemPaymentTokensUrlRequestBuilderUtilTest, GetPath) {
  // Act & Assert
  EXPECT_EQ("/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
            BuildRedeemPaymentTokensUrlPath(kWalletPaymentId));
}

}  // namespace brave_ads
