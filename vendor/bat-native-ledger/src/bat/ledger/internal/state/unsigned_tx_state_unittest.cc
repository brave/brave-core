/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/unsigned_tx_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UnsignedTxStateTest.*

namespace ledger {

TEST(UnsignedTxStateTest, ToJsonSerialization) {
  // Arrange
  UnsignedTxProperties unsigned_tx_properties;
  unsigned_tx_properties.amount = "Amount";
  unsigned_tx_properties.currency = "Currency";
  unsigned_tx_properties.destination = "Destination";

  // Act
  const UnsignedTxState unsigned_tx_state;
  const std::string json = unsigned_tx_state.ToJson(unsigned_tx_properties);

  // Assert
  UnsignedTxProperties expected_unsigned_tx_properties;
  unsigned_tx_state.FromJson(json, &expected_unsigned_tx_properties);
  EXPECT_EQ(expected_unsigned_tx_properties, unsigned_tx_properties);
}

TEST(UnsignedTxStateTest, FromJsonDeserialization) {
  // Arrange
  UnsignedTxProperties unsigned_tx_properties;
  unsigned_tx_properties.amount = "Amount";
  unsigned_tx_properties.currency = "Currency";
  unsigned_tx_properties.destination = "Destination";

  const std::string json = "{\"denomination\":{\"amount\":\"Amount\",\"currency\":\"Currency\"},\"destination\":\"Destination\"}";  // NOLINT

  // Act
  UnsignedTxProperties expected_unsigned_tx_properties;
  const UnsignedTxState unsigned_tx_state;
  unsigned_tx_state.FromJson(json, &expected_unsigned_tx_properties);

  // Assert
  EXPECT_EQ(expected_unsigned_tx_properties, unsigned_tx_properties);
}

TEST(UnsignedTxStateTest, FromJsonResponseDeserialization) {
  // Arrange
  UnsignedTxProperties unsigned_tx_properties;
  unsigned_tx_properties.amount = "Amount";
  unsigned_tx_properties.currency = "Currency";
  unsigned_tx_properties.destination = "Destination";

  const std::string json = "{\"unsignedTx\":{\"denomination\":{\"amount\":\"Amount\",\"currency\":\"Currency\"},\"destination\":\"Destination\"}}";  // NOLINT

  // Act
  UnsignedTxProperties expected_unsigned_tx_properties;
  const UnsignedTxState unsigned_tx_state;
  unsigned_tx_state.FromJsonResponse(json, &expected_unsigned_tx_properties);

  // Assert
  EXPECT_EQ(expected_unsigned_tx_properties, unsigned_tx_properties);
}

TEST(UnsignedTxStateTest, FromInvalidJsonResponseDeserialization) {
  // Arrange
  UnsignedTxProperties unsigned_tx_properties;
  unsigned_tx_properties.amount = "Amount";
  unsigned_tx_properties.currency = "Currency";
  unsigned_tx_properties.destination = "Destination";

  const std::string json = "FOOBAR";

  // Act
  UnsignedTxProperties expected_unsigned_tx_properties;
  const UnsignedTxState unsigned_tx_state;
  unsigned_tx_state.FromJsonResponse(json, &expected_unsigned_tx_properties);

  // Assert
  EXPECT_NE(expected_unsigned_tx_properties, unsigned_tx_properties);
}

}  // namespace ledger
