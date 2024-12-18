/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreateRewardConfirmationUrlRequestBuilderUtilTest, GetPath) {
  // Act & Assert
  EXPECT_EQ(
      R"(/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJzaWduYXR1cmUiOiJkaXZoV2dkM2w0TVpZeG9NV1VwL096RnRnc1VBdXlxcEo4R0J5emp2TFBFWkMzUGo3alhucWFXUmVWbU83cU5FNzNkWThaSk5ZQVBCaTZFV2pGMWpFZz09IiwidCI6Ii9tZlRBQWpIcldtQWxMaUVrdGJxTlMvZHhvTVZkbnoxZXNvVnBsUVVzN3lHL2FwQXEySzZPZVNUNmxCVEtGSm1PcTdyVjhRYlkvREYySEZSTWN6L0pRPT0ifQ==)",
      BuildCreateRewardConfirmationUrlPath(test::kTransactionId,
                                           test::kCredentialBase64Url));
}

}  // namespace brave_ads
