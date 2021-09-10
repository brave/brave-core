/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthDataParser, GetTransactionInfoFromDataTransfer) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  ASSERT_TRUE(GetTransactionInfoFromData(
      "0xa9059cbb000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000003fffffffffffffff",
      &tx_type, &tx_params, &tx_args));
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Transfer);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");

  // Missing a char for the last param
  EXPECT_FALSE(GetTransactionInfoFromData(
      "0xa9059cbb000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000003ffffffffffffff",
      &tx_type, &tx_params, &tx_args));
  // Missing the entire last param
  EXPECT_FALSE(
      GetTransactionInfoFromData("0xa9059cbb000000000000000000000000BFb30a082f6"
                                 "50C2A15D0632f0e87bE4F8e64460f",
                                 &tx_type, &tx_params, &tx_args));
  // No params
  EXPECT_FALSE(
      GetTransactionInfoFromData("0xa9059cbb", &tx_type, &tx_params, &tx_args));
  // Extra data
  EXPECT_FALSE(GetTransactionInfoFromData(
      "0xa9059cbb000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000003fffffffffffffff00",
      &tx_type, &tx_params, &tx_args));
}

TEST(EthDataParser, GetTransactionInfoFromDataApprove) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  ASSERT_TRUE(GetTransactionInfoFromData(
      "0x095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000003fffffffffffffff",
      &tx_type, &tx_params, &tx_args));
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");

  // Function case doesn't matter
  ASSERT_TRUE(GetTransactionInfoFromData(
      "0x095EA7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000003fffffffffffffff",
      &tx_type, &tx_params, &tx_args));
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");

  // Missing a char for the last param
  EXPECT_FALSE(GetTransactionInfoFromData(
      "0x095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000003ffffffffffffff",
      &tx_type, &tx_params, &tx_args));
  // Missing the entire last param
  EXPECT_FALSE(
      GetTransactionInfoFromData("0x095ea7b3000000000000000000000000BFb30a082f6"
                                 "50C2A15D0632f0e87bE4F8e64460f",
                                 &tx_type, &tx_params, &tx_args));
  // No params
  EXPECT_FALSE(
      GetTransactionInfoFromData("0x095ea7b3", &tx_type, &tx_params, &tx_args));
  // Extra data
  EXPECT_FALSE(GetTransactionInfoFromData(
      "0x095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000003fffffffffffffff00",
      &tx_type, &tx_params, &tx_args));
}

TEST(EthDataParser, GetTransactionInfoFromDataETHSend) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  ASSERT_TRUE(
      GetTransactionInfoFromData("0x0", &tx_type, &tx_params, &tx_args));
  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSend);
  ASSERT_EQ(tx_params.size(), 0UL);
  ASSERT_EQ(tx_args.size(), 0UL);
  ASSERT_TRUE(GetTransactionInfoFromData("", &tx_type, &tx_params, &tx_args));
  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSend);
  ASSERT_EQ(tx_params.size(), 0UL);
  ASSERT_EQ(tx_args.size(), 0UL);
}

TEST(EthDataParser, GetTransactionInfoFromDataOther) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;

  // No function hash
  EXPECT_FALSE(
      GetTransactionInfoFromData("0x1", &tx_type, &tx_params, &tx_args));
  EXPECT_EQ(tx_type, mojom::TransactionType::Other);
  EXPECT_TRUE(
      GetTransactionInfoFromData("0xaa0ffceb000000000000000000000000BFb30a082f6"
                                 "50C2A15D0632f0e87bE4F8e64460f",
                                 &tx_type, &tx_params, &tx_args));
  EXPECT_EQ(tx_type, mojom::TransactionType::Other);
  // Invaild input
  EXPECT_FALSE(
      GetTransactionInfoFromData("hello", &tx_type, &tx_params, &tx_args));
}

}  // namespace brave_wallet
