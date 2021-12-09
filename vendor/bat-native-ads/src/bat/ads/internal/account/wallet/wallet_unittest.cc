/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/wallet/wallet.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kId[] = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";
constexpr char kValidSeed[] = "x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=";
constexpr char kInvalidSeed[] = "y6vCwhJ6NUUWZ7tkHw76f0FIs9w8j-VylGC0rWd6gQ1=";

}  // namespace

class BatAdsWalletTest : public UnitTestBase {
 protected:
  BatAdsWalletTest() = default;

  ~BatAdsWalletTest() override = default;

  bool SetWallet(const std::string& id, const std::string& seed) {
    return wallet_.Set(id, seed);
  }

  WalletInfo GetWallet() const { return wallet_.Get(); }

  Wallet wallet_;
};

TEST_F(BatAdsWalletTest, SetWallet) {
  // Arrange

  // Act
  const bool success = SetWallet(kId, kValidSeed);

  // Assert
  EXPECT_TRUE(success);
}

TEST_F(BatAdsWalletTest, SetInvalidWallet) {
  // Arrange

  // Act
  const bool success = SetWallet(kId, kInvalidSeed);

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(BatAdsWalletTest, GetWallet) {
  // Arrange
  const bool success = SetWallet(kId, kValidSeed);
  ASSERT_TRUE(success);

  // Act
  const WalletInfo& wallet = GetWallet();

  // Assert
  WalletInfo expected_wallet;
  expected_wallet.id = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";
  expected_wallet.secret_key =
      "93052310477323AAE423A84BA32C68B1AE3B66B71952F6D8A69026E33BD817980621BF8B"
      "7B5F34B49E380F59179AE43C21B286473B28245B412DDB54632F150D";

  EXPECT_EQ(expected_wallet, wallet);
}

}  // namespace ads
