/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/wallet_info_state.h"
#include "base/base64.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=WalletInfoStateTest.*

namespace ledger {

TEST(WalletInfoStateTest, ToJsonSerialization) {
  // Arrange
  WalletInfoProperties wallet_info_properties;
  wallet_info_properties.payment_id = "PaymentId";
  wallet_info_properties.address_card_id = "AddressCardId";

  std::string key_info_seed;
  std::string base64_key_info_seed =
      "/kBv0C7wS4EBY3EIa780pYLrhryP3IWCfElIehufOFw=";
  ASSERT_TRUE(base::Base64Decode(base64_key_info_seed, &key_info_seed));
  wallet_info_properties.key_info_seed.assign(key_info_seed.begin(),
      key_info_seed.end());

  // Act
  const WalletInfoState wallet_info_state;
  const std::string json = wallet_info_state.ToJson(wallet_info_properties);

  // Assert
  WalletInfoProperties expected_wallet_info_properties;
  wallet_info_state.FromJson(json, &expected_wallet_info_properties);
  EXPECT_EQ(expected_wallet_info_properties, wallet_info_properties);
}

TEST(WalletInfoStateTest, FromJsonDeserialization) {
  // Arrange
  WalletInfoProperties wallet_info_properties;
  wallet_info_properties.payment_id = "PaymentId";
  wallet_info_properties.address_card_id = "AddressCardId";

  std::string key_info_seed;
  std::string base64_key_info_seed =
      "/kBv0C7wS4EBY3EIa780pYLrhryP3IWCfElIehufOFw=";
  ASSERT_TRUE(base::Base64Decode(base64_key_info_seed, &key_info_seed));
  wallet_info_properties.key_info_seed.assign(key_info_seed.begin(),
      key_info_seed.end());

  const std::string json = "{\"paymentId\":\"PaymentId\",\"addressCARD_ID\":\"AddressCardId\",\"keyInfoSeed\":\"/kBv0C7wS4EBY3EIa780pYLrhryP3IWCfElIehufOFw=\"}";  // NOLINT

  // Act
  WalletInfoProperties expected_wallet_info_properties;
  const WalletInfoState wallet_info_state;
  wallet_info_state.FromJson(json, &expected_wallet_info_properties);

  // Assert
  EXPECT_EQ(expected_wallet_info_properties, wallet_info_properties);
}

}  // namespace ledger
