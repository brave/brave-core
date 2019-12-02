/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "bat/ledger/internal/state/wallet_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=WalletStateTest.*

namespace ledger {

TEST(WalletStateTest, ToJsonSerialization) {
  // Arrange
  WalletProperties wallet_properties;
  wallet_properties.fee_amount = std::numeric_limits<double>::max();
  wallet_properties.parameters_choices = {
    5.0,
    10.0,
    15.0,
    20.0,
    25.0,
    50.0,
    100.0
  };

  // Act
  const WalletState wallet_state;
  std::string json = wallet_state.ToJson(wallet_properties);

  // Assert
  WalletProperties expected_wallet_properties;
  wallet_state.FromJson(json, &expected_wallet_properties);
  EXPECT_TRUE(expected_wallet_properties.Equals(wallet_properties));
}

TEST(WalletStateTest, FromJsonDeserialization) {
  // Arrange
  WalletProperties wallet_properties;
  wallet_properties.fee_amount = std::numeric_limits<double>::max();
  wallet_properties.parameters_choices = {
    5.0,
    10.0,
    15.0,
    20.0,
    25.0,
    50.0,
    100.0
  };

  const std::string json = "{\"fee_amount\":1.7976931348623157e308,\"parameters\":{\"adFree\":{\"fee\":{\"BAT\":1.7976931348623157e308},\"choices\":{\"BAT\":[5.0,10.0,15.0,20.0,25.0,50.0,100.0]}}}}";  // NOLINT

  // Act
  WalletProperties expected_wallet_properties;
  const WalletState wallet_state;
  wallet_state.FromJson(json, &expected_wallet_properties);

  // Assert
  EXPECT_TRUE(expected_wallet_properties.Equals(wallet_properties));
}

}  // namespace ledger
