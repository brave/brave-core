/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"

#include "brave/components/brave_ads/core/internal/account/wallet/test/wallet_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsWalletUtilTest, MaybeBuildWalletFromRecoverySeed) {
  // Act
  std::optional<WalletInfo> wallet = MaybeBuildWalletFromRecoverySeed(
      test::kWalletPaymentId, test::kWalletRecoverySeedBase64);
  ASSERT_TRUE(wallet);

  // Assert
  EXPECT_THAT(*wallet, ::testing::FieldsAre(test::kWalletPaymentId,
                                            test::kWalletPublicKey,
                                            test::kWalletSecretKey));
}

TEST(BraveAdsWalletUtilTest, DoesNotBuildWalletFromInvalidRecoverySeed) {
  // Act & Assert
  EXPECT_FALSE(MaybeBuildWalletFromRecoverySeed(
      test::kWalletPaymentId, test::kInvalidWalletRecoverySeed));
}

}  // namespace brave_ads
