/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreateNonRewardConfirmationUrlRequestBuilderUtilTest, GetPath) {
  // Act & Assert
  EXPECT_EQ("/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
            BuildCreateNonRewardConfirmationUrlPath(test::kTransactionId));
}

}  // namespace brave_ads
