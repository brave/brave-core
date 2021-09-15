/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_controller.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthTxControllerUnitTest, ValidateTxData) {
  std::string error_message;
  EXPECT_TRUE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));

  // Make sure if params are specified that they are valid hex strings
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("hello", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "hello", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "hello",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974", "hello",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "hello",
                         std::vector<uint8_t>()),
      &error_message));
  // to must not only be a valid hex string but also an address
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe",  // Invalid address
                         "hello", std::vector<uint8_t>()),
      &error_message));

  // To can't be missing if Data is missing
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974", "",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
}

TEST(EthTxControllerUnitTest, ValidateTxData1559) {
  std::string error_message;
  EXPECT_TRUE(EthTxController::ValidateTxData1559(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x1"),
      &error_message));

  // Can't specify both gas price and max fee per gas
  EXPECT_FALSE(EthTxController::ValidateTxData1559(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "0x1", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x1"),
      &error_message));
}

}  //  namespace brave_wallet
