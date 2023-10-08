/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsWalletUtilTest, ToWallet) {
  // Act & Assert
  WalletInfo expected_wallet;
  expected_wallet.payment_id = kWalletPaymentId;
  expected_wallet.public_key = kWalletPublicKey;
  expected_wallet.secret_key = kWalletSecretKey;
  EXPECT_EQ(expected_wallet, ToWallet(kWalletPaymentId, kWalletRecoverySeed));
}

TEST(BraveAdsWalletUtilTest, ToInvalidWallet) {
  // Act & Assert
  EXPECT_FALSE(ToWallet(kWalletPaymentId, kInvalidWalletRecoverySeed));
}

}  // namespace brave_ads
