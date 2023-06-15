/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/wallet/wallet.h"

#include "base/base64.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsWalletTest, SetWallet) {
  // Arrange
  Wallet wallet;

  const absl::optional<std::vector<uint8_t>> raw_recovery_seed =
      base::Base64Decode(kWalletRecoverySeed);
  ASSERT_TRUE(raw_recovery_seed);

  // Act
  const bool success = wallet.Set(kWalletPaymentId, *raw_recovery_seed);
  ASSERT_TRUE(success);

  // Assert
  WalletInfo expected_wallet;
  expected_wallet.payment_id = kWalletPaymentId;
  expected_wallet.public_key = kWalletPublicKey;
  expected_wallet.secret_key = kWalletSecretKey;

  EXPECT_EQ(expected_wallet, wallet.Get());
}

TEST(BraveAdsWalletTest, SetFromAnotherWallet) {
  // Arrange
  Wallet another_wallet;
  const absl::optional<std::vector<uint8_t>> raw_recovery_seed =
      base::Base64Decode(kWalletRecoverySeed);
  ASSERT_TRUE(raw_recovery_seed);
  const bool success = another_wallet.Set(kWalletPaymentId, *raw_recovery_seed);
  ASSERT_TRUE(success);

  // Act
  Wallet wallet;
  wallet.SetFrom(another_wallet.Get());

  // Assert
  WalletInfo expected_wallet;
  expected_wallet.payment_id = kWalletPaymentId;
  expected_wallet.public_key = kWalletPublicKey;
  expected_wallet.secret_key = kWalletSecretKey;

  EXPECT_EQ(expected_wallet, wallet.Get());
}

}  // namespace brave_ads
