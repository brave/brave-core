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

void TestGetTransactionInfoFromData(const std::string& data,
                                    bool expected_result,
                                    mojom::TransactionType expected_tx_type,
                                    std::vector<std::string> expected_tx_params,
                                    std::vector<std::string> expected_tx_args) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;

  ASSERT_EQ(expected_result,
            GetTransactionInfoFromData(data, &tx_type, &tx_params, &tx_args));
  ASSERT_EQ(tx_type, expected_tx_type);
  ASSERT_EQ(tx_params.size(), expected_tx_params.size());
  ASSERT_EQ(tx_args.size(), expected_tx_args.size());

  for (int i = 0; i < static_cast<int>(tx_params.size()); i++) {
    ASSERT_EQ(tx_params[i], expected_tx_params[i]);
  }

  for (int i = 0; i < static_cast<int>(tx_args.size()); i++) {
    ASSERT_EQ(tx_args[i], expected_tx_args[i]);
  }
}

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

TEST(EthDataParser, GetTransactionInfoFromDataERC721TransferFrom) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  ASSERT_TRUE(GetTransactionInfoFromData(
      "0x23b872dd000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a000000"
      "000000000000000000000000000000000000000000000000000000000f",
      &tx_type, &tx_params, &tx_args));
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721TransferFrom);
  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "address");
  EXPECT_EQ(tx_params[2], "uint256");
  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0], "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f");
  EXPECT_EQ(tx_args[1], "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a");
  EXPECT_EQ(tx_args[2], "0xf");

  ASSERT_TRUE(GetTransactionInfoFromData(
      "0x42842e0e000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a000000"
      "000000000000000000000000000000000000000000000000000000000f",
      &tx_type, &tx_params, &tx_args));
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721SafeTransferFrom);
  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "address");
  EXPECT_EQ(tx_params[2], "uint256");
  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0], "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f");
  EXPECT_EQ(tx_args[1], "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a");
  EXPECT_EQ(tx_args[2], "0xf");

  // Missing a char for the last param
  EXPECT_FALSE(GetTransactionInfoFromData(
      "0x23b872dd000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a000000"
      "000000000000000000000000000000000000000000000000000000000",
      &tx_type, &tx_params, &tx_args));
  // Missing the entire last param
  EXPECT_FALSE(GetTransactionInfoFromData(
      "0x23b872dd000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a",
      &tx_type, &tx_params, &tx_args));
  // No params
  EXPECT_FALSE(
      GetTransactionInfoFromData("0x23b872dd", &tx_type, &tx_params, &tx_args));
  // Extra data
  EXPECT_FALSE(GetTransactionInfoFromData(
      "0x23b872dd000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a000000"
      "000000000000000000000000000000000000000000000000000000000f00",
      &tx_type, &tx_params, &tx_args));
}

TEST(EthDataParser, GetTransactionInfoFromDataERC1155SafeTransferFrom) {
  // Valid empty bytes
  TestGetTransactionInfoFromData(
      "0xf242432a"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000000",
      true, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{"address", "address", "uint256", "uint256",
                               "bytes"},
      std::vector<std::string>{"0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x0", "0x1", "0x"});

  // Valid empty bytes with extra tail data
  TestGetTransactionInfoFromData(
      "0xf242432a"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      true, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{"address", "address", "uint256", "uint256",
                               "bytes"},
      std::vector<std::string>{"0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x0", "0x1", "0x"});

  // Valid non empty bytes
  TestGetTransactionInfoFromData(
      "0xf242432a"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000010"
      "0000000000000000000000000000000100000000000000000000000000000000",
      true, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{"address", "address", "uint256", "uint256",
                               "bytes"},
      std::vector<std::string>{"0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x0", "0x1",
                               "0x00000000000000000000000000000001"});

  // Invalid non empty bytes (length parameter too large)
  TestGetTransactionInfoFromData(
      "0xf242432a"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000030"
      "0000000000000000000000000000000100000000000000000000000000000000",
      false, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{}, std::vector<std::string>{});

  // Invalid (missing length)
  TestGetTransactionInfoFromData(
      "0xf242432a"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0",
      false, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{}, std::vector<std::string>{});

  // Invalid (incorrect offset)
  TestGetTransactionInfoFromData(
      "0xf242432a"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000000",
      false, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{}, std::vector<std::string>{});

  // Invalid (no params)
  TestGetTransactionInfoFromData(
      "0xf242432a", false, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{}, std::vector<std::string>{});
}

TEST(EthDataParser, GetTransactionInfoFromDataOther) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;

  // No function hash
  EXPECT_TRUE(
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
