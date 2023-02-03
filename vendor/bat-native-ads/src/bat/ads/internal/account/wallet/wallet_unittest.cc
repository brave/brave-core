/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/wallet/wallet.h"

#include "absl/types/optional.h"
#include "base/base64.h"
#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsWalletTest, SetWallet) {
  // Arrange
  Wallet wallet;

  const absl::optional<std::vector<uint8_t>> raw_recovery_seed =
      base::Base64Decode(GetWalletRecoverySeedForTesting());
  ASSERT_TRUE(raw_recovery_seed);

  // Act
  const bool success =
      wallet.Set(GetWalletPaymentIdForTesting(), *raw_recovery_seed);
  ASSERT_TRUE(success);

  // Assert
  WalletInfo expected_wallet;
  expected_wallet.payment_id = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";
  expected_wallet.public_key = "BiG/i3tfNLSeOA9ZF5rkPCGyhkc7KCRbQS3bVGMvFQ0=";
  expected_wallet.secret_key =
      "kwUjEEdzI6rkI6hLoyxosa47ZrcZUvbYppAm4zvYF5gGIb+"
      "Le180tJ44D1kXmuQ8IbKGRzsoJFtBLdtUYy8VDQ==";

  EXPECT_EQ(expected_wallet, wallet.Get());
}

}  // namespace ads
