/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
void TestGetTransactionInfoFromData(const std::vector<uint8_t>& data,
                                    mojom::TransactionType expected_tx_type,
                                    std::vector<std::string> expected_tx_params,
                                    std::vector<std::string> expected_tx_args) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;

  auto result = GetTransactionInfoFromData(data);
  ASSERT_NE(result, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *result;
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
}  // namespace

TEST(EthDataParser, GetTransactionInfoFromDataTransfer) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // OK: well-formed ERC20Transfer
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa9059cbb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Transfer);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");

  // KO: missing a byte for the last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa9059cbb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffff",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // KO: missing the entire last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa9059cbb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // KO: no params
  ASSERT_TRUE(PrefixedHexStringToBytes("0xa9059cbb", &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // OK: extra data
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa9059cbb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff"
      "00",
      &data));
  tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Transfer);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");
}

TEST(EthDataParser, GetTransactionInfoFromDataApprove) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // OK: well-formed ERC20Approve
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095ea7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");

  // OK: function case doesn't matter
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095EA7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff",
      &data));
  tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");

  // KO: missing a byte for the last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095ea7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffff",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // KO: missing the entire last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095ea7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // KO: no params
  ASSERT_TRUE(PrefixedHexStringToBytes("0x095ea7b3", &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // OK: extra data
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095ea7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff"
      "00",
      &data));
  tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "uint256");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0x3fffffffffffffff");
}

TEST(EthDataParser, GetTransactionInfoFromDataETHSend) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;

  std::vector<uint8_t> data;
  ASSERT_TRUE(PrefixedHexStringToBytes("0x0", &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSend);
  ASSERT_EQ(tx_params.size(), 0UL);
  ASSERT_EQ(tx_args.size(), 0UL);

  tx_info = GetTransactionInfoFromData({});
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSend);
  ASSERT_EQ(tx_params.size(), 0UL);
  ASSERT_EQ(tx_args.size(), 0UL);
}

TEST(EthDataParser, GetTransactionInfoFromDataERC721TransferFrom) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // OK: well-formed ERC721TransferFrom
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x23b872dd"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a"
      "000000000000000000000000000000000000000000000000000000000000000f",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721TransferFrom);
  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "address");
  EXPECT_EQ(tx_params[2], "uint256");
  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460a");
  EXPECT_EQ(tx_args[2], "0xf");

  // OK: well-formed ERC721SafeTransferFrom
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x42842e0e"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a"
      "000000000000000000000000000000000000000000000000000000000000000f",
      &data));
  tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721SafeTransferFrom);
  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "address");
  EXPECT_EQ(tx_params[2], "uint256");
  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460a");
  EXPECT_EQ(tx_args[2], "0xf");

  // KO: missing a byte for the last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x23b872dd"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a"
      "00000000000000000000000000000000000000000000000000000000000000",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // KO: missing the entire last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x23b872dd"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // KO: no params
  ASSERT_TRUE(PrefixedHexStringToBytes("0x23b872dd", &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // OK: extra data
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x23b872dd"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a"
      "000000000000000000000000000000000000000000000000000000000000000f"
      "00",
      &data));
  tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721TransferFrom);
  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "address");
  EXPECT_EQ(tx_params[2], "uint256");
  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460a");
  EXPECT_EQ(tx_args[2], "0xf");
}

TEST(EthDataParser, GetTransactionInfoFromDataERC1155SafeTransferFrom) {
  std::vector<uint8_t> data;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xf242432a"  // function selector
      /*********************** HEAD (32x5 bytes) **********************/
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"

      /***************************** TAIL *****************************/
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  TestGetTransactionInfoFromData(
      data, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{"address", "address", "uint256", "uint256",
                               "bytes"},
      std::vector<std::string>{"0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x0", "0x1", "0x"});

  // Valid empty bytes with extra tail data
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xf242432a"  // function selector
      /*********************** HEAD (32x5 bytes) **********************/
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"

      /***************************** TAIL *****************************/
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  TestGetTransactionInfoFromData(
      data, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{"address", "address", "uint256", "uint256",
                               "bytes"},
      std::vector<std::string>{"0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x0", "0x1", "0x"});

  // Valid non-empty bytes
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xf242432a"  // function selector
      /*********************** HEAD (32x5 bytes) **********************/
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"

      /***************************** TAIL *****************************/
      "0000000000000000000000000000000000000000000000000000000000000010"
      "00000000000000000000000000000001"

      // extraneous calldata
      "00000000000000000000000000000000",
      &data));
  TestGetTransactionInfoFromData(
      data, mojom::TransactionType::ERC1155SafeTransferFrom,
      std::vector<std::string>{"address", "address", "uint256", "uint256",
                               "bytes"},
      std::vector<std::string>{"0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                               "0x0", "0x1",
                               "0x00000000000000000000000000000001"});

  // Invalid non-empty bytes (length parameter too large)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xf242432a"  // function selector
      /*********************** HEAD (32x5 bytes) **********************/
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"

      /***************************** TAIL *****************************/
      "0000000000000000000000000000000000000000000000000000000000000030"
      "0000000000000000000000000000000100000000000000000000000000000000",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // Invalid (missing length)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xf242432a"  // function selector
      /*********************** HEAD (32x5 bytes) **********************/
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // Invalid (incorrect offset)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xf242432a"  // function selector
      /*********************** HEAD (32x5 bytes) **********************/
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d85977fe52"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000020"

      /***************************** TAIL *****************************/
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));

  // Invalid (no params)
  ASSERT_TRUE(PrefixedHexStringToBytes("0xf242432a", &data));
  EXPECT_FALSE(GetTransactionInfoFromData(data));
}

TEST(EthDataParser, GetTransactionInfoFromDataOther) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;

  std::vector<uint8_t> data;

  // No function hash
  auto tx_info = GetTransactionInfoFromData(std::vector<uint8_t>{0x1});
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  EXPECT_EQ(tx_type, mojom::TransactionType::Other);

  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xaa0ffceb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;
  EXPECT_EQ(tx_type, mojom::TransactionType::Other);
}

TEST(EthDataParser, GetTransactionInfoFromDataSellEthForTokenToUniswapV3) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // TXN: WETH → STG
  // sellEthForTokenToUniswapV3(bytes encodedPath,
  //                            uint256 minBuyAmount,
  //                            address recipient)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x3598d8ab"  // function selector
      /*********************** HEAD (32x3 bytes) **********************/
      "0000000000000000000000000000000000000000000000000000000000000060"
      "0000000000000000000000000000000000000000000000030c1a39b13e25f498"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /***************************** TAIL *****************************/
      // calldata reference position for encodedPath
      "000000000000000000000000000000000000000000000000000000000000002b"
      "c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"    // WETH
      "002710"                                      // POOL FEE
      "af5191b0de278c7286d6c7cc6ab6bb8a73ba2cd6"    // STG
      "000000000000000000000000000000000000000000"  // recipient address

      // extraneous tail segment to be ignored
      "869584cd0000000000000000000000003ce37278de6388532c3949ce4e886f36"
      "5b14fb560000000000000000000000000000000000000000000000f7834ab14c"
      "623f4f93",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);

  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "bytes");
  EXPECT_EQ(tx_params[1], "uint256");
  EXPECT_EQ(tx_params[2], "uint256");

  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0],
            "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
            "af5191b0de278c7286d6c7cc6ab6bb8a73ba2cd6");
  EXPECT_EQ(tx_args[1], "");
  EXPECT_EQ(tx_args[2], "0x30c1a39b13e25f498");
}

TEST(EthDataParser, GetTransactionInfoFromDataSellTokenForEthToUniswapV3) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // TXN: RSS3 → USDC → WETH
  // sellTokenForEthToUniswapV3(bytes encodedPath,
  //                            uint256 sellAmount,
  //                            uint256 minBuyAmount,
  //                            address recipient)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x803ba26d"  // function selector
      /*********************** HEAD (32x4 bytes) **********************/
      "0000000000000000000000000000000000000000000000000000000000000080"
      "0000000000000000000000000000000000000000000000821ab0d44149800000"
      "0000000000000000000000000000000000000000000000000248b3366b6ffd46"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /***************************** TAIL *****************************/
      // calldata reference position for encodedPath
      "0000000000000000000000000000000000000000000000000000000000000042"
      "c98d64da73a6616c42117b582e832812e7b8d57f"  // RSS3
      "000bb8"                                    // POOL FEE
      "a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"  // USDC
      "0001f4"                                    // POOL FEE
      "c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"  // WETH

      // extraneous tail segment to be ignored
      "0000000000000000000000000000000000000000000000000000000000008695"
      "84cd00000000000000000000000086003b044f70dac0abc80ac8957305b63708"
      "93ed0000000000000000000000000000000000000000000000c42194bea56247"
      "eafe",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);

  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "bytes");
  EXPECT_EQ(tx_params[1], "uint256");
  EXPECT_EQ(tx_params[2], "uint256");

  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0],
            "0xc98d64da73a6616c42117b582e832812e7b8d57f"  // RSS3
            "a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"    // USDC
            "c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2");  // WETH
  EXPECT_EQ(tx_args[1], "0x821ab0d44149800000");
  EXPECT_EQ(tx_args[2], "0x248b3366b6ffd46");
}

TEST(EthDataParser, GetTransactionInfoFromDataSellTokenForTokenToUniswapV3) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // TXN: COW → WETH → USDC
  // sellTokenForTokenToUniswapV3(bytes encodedPath,
  //                              uint256 sellAmount,
  //                              uint256 minBuyAmount,
  //                              address recipient)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6af479b2"  // function selector

      /*********************** HEAD (32x4 bytes) **********************/
      "0000000000000000000000000000000000000000000000000000000000000080"
      "00000000000000000000000000000000000000000000004d12b6295c69ddebd5"
      "000000000000000000000000000000000000000000000000000000003b9aca00"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /***************************** TAIL *****************************/
      // calldata reference position for encodedPath
      "0000000000000000000000000000000000000000000000000000000000000042"
      "def1ca1fb7fbcdc777520aa7f396b4e015f497ab"  // COW
      "002710"                                    // POOL FEE
      "c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"  // WETH
      "0001f4"                                    // POOL FEE
      "a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"  // USDC

      // extraneous tail segment to be ignored
      "000000000000000000000000000000000000000000000000000000000000869584cd0000"
      "0000000000000000000086003b044f70dac0abc80ac8957305b6370893ed000000000000"
      "0000000000000000000000000000000000495d35e8bf6247f2f1",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);

  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "bytes");
  EXPECT_EQ(tx_params[1], "uint256");
  EXPECT_EQ(tx_params[2], "uint256");

  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0],
            "0xdef1ca1fb7fbcdc777520aa7f396b4e015f497ab"  // COW
            "c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"    // WETH
            "a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");  // USDC
  EXPECT_EQ(tx_args[1], "0x4d12b6295c69ddebd5");
  EXPECT_EQ(tx_args[2], "0x3b9aca00");
}

TEST(EthDataParser, GetTransactionInfoFromDataSellToUniswap) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // TXN: USDC → WETH → LDO
  // sellToUniswap(address[] tokens,
  //               uint256 sellAmount,
  //               uint256 minBuyAmount,
  //               bool isSushi)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xd9627aa4"  // function selector

      /*********************** HEAD (32x4 bytes) **********************/
      // calldata pointer to tokens
      "0000000000000000000000000000000000000000000000000000000000000080"
      "0000000000000000000000000000000000000000000000000000000077359400"
      "000000000000000000000000000000000000000000000016b28ec6ba93b8bb17"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /***************************** TAIL *****************************/
      // calldata reference position for tokens
      "0000000000000000000000000000000000000000000000000000000000000003"
      "000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"
      "000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
      "0000000000000000000000005a98fcbea516cf06857215779fd812ca3bef1b32"

      // extraneous tail segment to be ignored
      "869584cd00000000000000000000000086003b044f70dac0abc80ac8957305b63"
      "70893ed0000000000000000000000000000000000000000000000da92815dbd62"
      "4a716a",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);

  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "bytes");
  EXPECT_EQ(tx_params[1], "uint256");
  EXPECT_EQ(tx_params[2], "uint256");

  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0],
            "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"  // USDC
            "c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"    // WETH
            "5a98fcbea516cf06857215779fd812ca3bef1b32");  // LDO
  EXPECT_EQ(tx_args[1], "0x77359400");
  EXPECT_EQ(tx_args[2], "0x16b28ec6ba93b8bb17");
}

TEST(EthDataParser, GetTransactionInfoFromDataTransformERC20) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // TXN: USDT → ETH
  // transformERC20(address inputToken,
  //                address outputToken,
  //                uint256 inputTokenAmount,
  //                uint256 minOutputTokenAmount,
  //                (uint32,bytes)[] transformations)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x415565b0"  // function selector
      /*********************** HEAD (32x5 bytes) **********************/
      "000000000000000000000000dac17f958d2ee523a2206206994597c13d831ec7"
      "000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      "00000000000000000000000000000000000000000000000000000004a817c7ff"
      "00000000000000000000000000000000000000000000000055c75b6fa8d5f0d4"
      "00000000000000000000000000000000000000000000000000000000000000a0"

      /*********************** TAIL (truncated) ***********************/
      // calldata reference position for transformations
      "0000000000000000000000000000000000000000000000000000000000000004"
      "0000000000000000000000000000000000000000000000000000000000000080"
      "00000000000000000000000000000000000000000000000000000000000003e0"
      "0000000000000000000000000000000000000000000000000000000000000480",
      &data));
  auto tx_info = GetTransactionInfoFromData(data);
  ASSERT_NE(tx_info, absl::nullopt);
  std::tie(tx_type, tx_params, tx_args) = *tx_info;

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);

  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "bytes");
  EXPECT_EQ(tx_params[1], "uint256");
  EXPECT_EQ(tx_params[2], "uint256");

  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0],
            "0xdac17f958d2ee523a2206206994597c13d831ec7"
            "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(tx_args[1], "0x4a817c7ff");
  EXPECT_EQ(tx_args[2], "0x55c75b6fa8d5f0d4");
}

}  // namespace brave_wallet
