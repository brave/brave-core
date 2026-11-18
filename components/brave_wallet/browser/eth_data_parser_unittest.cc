/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kNotASwapRouter[] = "0x4988ec37b0ba5d90016b3af633c11da45f67cdf0";
constexpr char kCowSwapEthFlow[] = "0xbA3cB449bD2B4ADddBc894D8697F5170800EAdeC";
constexpr char kCowSwapEthFlowLegacyMainnet[] =
    "0x40A50cf069e992AA4536211B23F286eF88752187";
constexpr char kLiFiDiamond[] = "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE";
constexpr char kLiFiDiamondZkSync[] =
    "0x341e94069f53234fE6DabeF707aD424830525715";
constexpr char kSquidRouter[] = "0xce16F69375520ab01377ce7B88f5BA8C48F8D666";

// Decodes |data| as sent to a contract that is not a swap router.
std::optional<DecodedTransaction> GetNonSwapTxInfo(
    const std::vector<uint8_t>& data) {
  return GetTransactionInfoFromData(mojom::kMainnetChainId, kNotASwapRouter,
                                    data);
}

// Swap calldata sent to |to| on |chain_id| must be reported as a plain
// contract call, without swap info.
void ExpectNotSwap(std::string_view chain_id,
                   std::string_view to,
                   const std::vector<uint8_t>& data) {
  auto tx_info = GetTransactionInfoFromData(chain_id, to, data);
  ASSERT_NE(tx_info, std::nullopt);
  EXPECT_EQ(std::get<0>(*tx_info), mojom::TransactionType::Other);
  EXPECT_TRUE(std::get<1>(*tx_info).empty());
  EXPECT_TRUE(std::get<2>(*tx_info).empty());
  EXPECT_FALSE(std::get<3>(*tx_info));
}

// Swap calldata sent to |to| on |chain_id| must be reported as a swap.
void ExpectSwap(std::string_view chain_id,
                std::string_view to,
                const std::vector<uint8_t>& data) {
  auto tx_info = GetTransactionInfoFromData(chain_id, to, data);
  ASSERT_NE(tx_info, std::nullopt);
  EXPECT_EQ(std::get<0>(*tx_info), mojom::TransactionType::ETHSwap);
  EXPECT_TRUE(std::get<3>(*tx_info));
}

void TestGetTransactionInfoFromData(
    const std::vector<uint8_t>& data,
    mojom::TransactionType expected_tx_type,
    std::vector<std::string> expected_tx_params,
    std::vector<std::string> expected_tx_args,
    mojom::SwapInfoPtr expected_swap_info = nullptr) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;

  auto result = GetNonSwapTxInfo(data);
  ASSERT_NE(result, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*result);
  ASSERT_EQ(tx_type, expected_tx_type);
  ASSERT_EQ(tx_params.size(), expected_tx_params.size());
  ASSERT_EQ(tx_args.size(), expected_tx_args.size());
  ASSERT_EQ(swap_info, expected_swap_info);

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
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // OK: well-formed ERC20Transfer
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa9059cbb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff",
      &data));
  auto tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Transfer);
  EXPECT_FALSE(swap_info);
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
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // KO: missing the entire last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa9059cbb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // KO: no params
  ASSERT_TRUE(PrefixedHexStringToBytes("0xa9059cbb", &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // OK: extra data
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa9059cbb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff"
      "00",
      &data));
  tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Transfer);
  EXPECT_FALSE(swap_info);
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
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // OK: well-formed ERC20Approve
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095ea7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff",
      &data));
  auto tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  EXPECT_FALSE(swap_info);
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
  tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  EXPECT_FALSE(swap_info);
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
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // KO: missing the entire last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095ea7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // KO: no params
  ASSERT_TRUE(PrefixedHexStringToBytes("0x095ea7b3", &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // OK: extra data
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x095ea7b3"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000003fffffffffffffff"
      "00",
      &data));
  tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC20Approve);
  EXPECT_FALSE(swap_info);
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
  mojom::SwapInfoPtr swap_info;

  std::vector<uint8_t> data;
  ASSERT_TRUE(PrefixedHexStringToBytes("0x0", &data));
  auto tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSend);
  EXPECT_FALSE(swap_info);
  ASSERT_EQ(tx_params.size(), 0UL);
  ASSERT_EQ(tx_args.size(), 0UL);

  tx_info = GetNonSwapTxInfo({});
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSend);
  EXPECT_FALSE(swap_info);
  ASSERT_EQ(tx_params.size(), 0UL);
  ASSERT_EQ(tx_args.size(), 0UL);
}

TEST(EthDataParser, GetTransactionInfoFromDataERC721TransferFrom) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // OK: well-formed ERC721TransferFrom
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x23b872dd"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a"
      "000000000000000000000000000000000000000000000000000000000000000f",
      &data));
  auto tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721TransferFrom);
  EXPECT_FALSE(swap_info);
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
  tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721SafeTransferFrom);
  EXPECT_FALSE(swap_info);
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
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // KO: missing the entire last param
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x23b872dd"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a",
      &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // KO: no params
  ASSERT_TRUE(PrefixedHexStringToBytes("0x23b872dd", &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // OK: extra data
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x23b872dd"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a"
      "000000000000000000000000000000000000000000000000000000000000000f"
      "00",
      &data));
  tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721TransferFrom);
  EXPECT_FALSE(swap_info);
  ASSERT_EQ(tx_params.size(), 3UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "address");
  EXPECT_EQ(tx_params[2], "uint256");
  ASSERT_EQ(tx_args.size(), 3UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460a");
  EXPECT_EQ(tx_args[2], "0xf");
}

TEST(EthDataParser, GetTransactionInfoFromDataERC721SetApprovalForAll) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // OK: well-formed setApprovalForAll granting approval.
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa22cb465"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000000000000000000001",
      &data));
  auto tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721SetApprovalForAll);
  EXPECT_FALSE(swap_info);
  ASSERT_EQ(tx_params.size(), 2UL);
  EXPECT_EQ(tx_params[0], "address");
  EXPECT_EQ(tx_params[1], "bool");
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0x1");

  // OK: well-formed setApprovalForAll revoking approval.
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa22cb465"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  ASSERT_EQ(tx_type, mojom::TransactionType::ERC721SetApprovalForAll);
  ASSERT_EQ(tx_args.size(), 2UL);
  EXPECT_EQ(tx_args[0], "0xbfb30a082f650c2a15d0632f0e87be4f8e64460f");
  EXPECT_EQ(tx_args[1], "0x0");

  // KO: non-canonical bool value (neither 0 nor 1).
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa22cb465"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f"
      "0000000000000000000000000000000000000000000000000000000000000002",
      &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // KO: missing the bool param.
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xa22cb465"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));
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
  EXPECT_FALSE(GetNonSwapTxInfo(data));

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
  EXPECT_FALSE(GetNonSwapTxInfo(data));

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
  EXPECT_FALSE(GetNonSwapTxInfo(data));

  // Invalid (no params)
  ASSERT_TRUE(PrefixedHexStringToBytes("0xf242432a", &data));
  EXPECT_FALSE(GetNonSwapTxInfo(data));
}

TEST(EthDataParser, GetTransactionInfoFromDataOther) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;

  std::vector<uint8_t> data;

  // No function hash
  auto tx_info = GetNonSwapTxInfo(std::vector<uint8_t>{0x1});
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  EXPECT_EQ(tx_type, mojom::TransactionType::Other);
  EXPECT_FALSE(swap_info);

  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xaa0ffceb"
      "000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);
  EXPECT_EQ(tx_type, mojom::TransactionType::Other);
  EXPECT_FALSE(swap_info);
}

TEST(EthDataParser, GetTransactionInfoFromDataCowOrderSellEth) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // TXN: XDAI → USDC
  // Function:
  // createOrder((address buyToken,
  //              address receiver,
  //              uint256 sellAmount,
  //              uint256 buyAmount,
  //              bytes32 appData,
  //              uint256 feeAmount,
  //              uint32 validTo,
  //              bool partiallyFillable,
  //              int64 quoteId))

  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x322bba21"  // function selector

      /***************************** HEAD ****************************/
      /************************ TUPLE INDEX 0 ************************/
      // buyToken
      "000000000000000000000000ddafbb505ad214d7b80b1f830fccc89b60fb7a83"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // sellAmount
      "000000000000000000000000000000000000000000000000004967cb9ebd8176"
      // buyAmount
      "0000000000000000000000000000000000000000000000000000000000004f1e"
      // appData
      "c21ba2efa76e703f0a9a496e09ea7d0e66d907a47ba8f109a3a760720504ab32"
      // feeAmount
      "000000000000000000000000000000000000000000000000000107c0fe0dc060"
      // validTo
      "00000000000000000000000000000000000000000000000000000000650b4580"
      // partiallyFillable
      "0000000000000000000000000000000000000000000000000000000000000000"
      // quoteId
      "000000000000000000000000000000000000000000000000000000000332b123",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kGnosisChainId, kCowSwapEthFlow, data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  EXPECT_EQ(tx_params.size(), 0UL);
  EXPECT_EQ(tx_args.size(), 0UL);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // XDAI
  EXPECT_EQ(swap_info->source_amount,
            "0x4967cb9ebd8176");  // 0.02066179753911948 XDAI

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0xddafbb505ad214d7b80b1f830fccc89b60fb7a83");  // USDC
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min, "0x4f1e");  // 0.020254 USDC

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kCowSwap);

  // Router addresses are compared case-insensitively.
  ExpectSwap(mojom::kGnosisChainId, base::ToLowerASCII(kCowSwapEthFlow), data);

  // The legacy EthFlow is only trusted on Ethereum.
  ExpectSwap(mojom::kMainnetChainId, kCowSwapEthFlowLegacyMainnet, data);
  ExpectNotSwap(mojom::kGnosisChainId, kCowSwapEthFlowLegacyMainnet, data);

  // Not sent to EthFlow.
  ExpectNotSwap(mojom::kGnosisChainId, kNotASwapRouter, data);
  ExpectNotSwap(mojom::kGnosisChainId, "", data);

  // EthFlow on a chain where CoW Protocol does not serve orders.
  ExpectNotSwap(mojom::kOptimismMainnetChainId, kCowSwapEthFlow, data);

  // Router of another swap provider.
  ExpectNotSwap(mojom::kGnosisChainId, kLiFiDiamond, data);
}

TEST(EthDataParser, GetTransactionInfoFromFilForward) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;

  std::vector<uint8_t> data;
  ASSERT_TRUE(
      PrefixedHexStringToBytes("0xd948d468"  // forward(bytes)
                               "00000000000000000000000000000000000000000000000"
                               "00000000000000020"  // bytes offset
                               "00000000000000000000000000000000000000000000000"
                               "00000000000000015"  // bytes length
                               "01d15cf6d7364d8b4dab9d90dc5699d1a78cf729c100000"
                               "00000000000000000",  // bytes content
                               &data));
  auto tx_info = GetNonSwapTxInfo(data);
  ASSERT_NE(tx_info, std::nullopt);
  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHFilForwarderTransfer);
  EXPECT_FALSE(swap_info);
  ASSERT_EQ(tx_params.size(), 1UL);
  ASSERT_EQ(tx_args.size(), 1UL);
  EXPECT_EQ(tx_params[0], "bytes");
  EXPECT_EQ(tx_args[0], "0x01d15cf6d7364d8b4dab9d90dc5699d1a78cf729c1");
}

TEST(EthDataParser, GetTransactionInfoFromDataLiFiSwapTokensGeneric) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // TXN: token → token
  // Function:
  // swapTokensGeneric(bytes32 transactionId,
  //                   string integrator,
  //                   string referrer,
  //                   address receiver,
  //                   uint256 minAmountOut,
  //                   (address callTo,
  //                    address approveTo,
  //                    address sendingAssetId,
  //                    address receivingAssetId,
  //                    uint256 fromAmount,
  //                    bytes callData,
  //                    bool requiresDeposit)[] swapData)

  // Swap 0.504913 USDC.e → 0.6797397017301765 MATIC
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x4630a0d8"  // function selector

      /***************************** HEAD ****************************/
      // transactionId
      "a45ad3e99c879cbd3103bcb2a36fc940d9c017500e587fc63b796ff8f4d28d89"
      // integrator (offset)
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000100"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmountOut
      "000000000000000000000000000000000000000000000000096eeba8455b6e35"

      // offset to start data part of swapData
      "0000000000000000000000000000000000000000000000000000000000000160"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000"

      // offset to start data part of referrer
      // size of referrer string
      "000000000000000000000000000000000000000000000000000000000000002a"
      // referrer string
      "3078303030303030303030303030303030303030303030303030303030303030"
      "3030303030303030303000000000000000000000000000000000000000000000"

      // size(swapData) = 2
      "0000000000000000000000000000000000000000000000000000000000000002"
      // swapData[0] offset
      "0000000000000000000000000000000000000000000000000000000000000040"
      // swapData[1] offset
      "00000000000000000000000000000000000000000000000000000000000001e0"

      /************************** swapData[0] *************************/
      // callTo
      "000000000000000000000000bd6c7b0d2f68c2b7805d88388319cfb6ecb50ea9"
      // approveTo
      "000000000000000000000000bd6c7b0d2f68c2b7805d88388319cfb6ecb50ea9"
      // sendingAssetId
      "0000000000000000000000002791bca1f2de4661ed88a30c99a7a9449aa84174"
      // receivingAssetId
      "0000000000000000000000002791bca1f2de4661ed88a30c99a7a9449aa84174"
      // fromAmount
      "000000000000000000000000000000000000000000000000000000000007b451"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000001"

      "0000000000000000000000000000000000000000000000000000000000000084"
      "eedd56e10000000000000000000000002791bca1f2de4661ed88a30c99a7a944"
      "9aa8417400000000000000000000000000000000000000000000000000000000"
      "00000dcd00000000000000000000000000000000000000000000000000000000"
      "00000373000000000000000000000000bd9420a98a7bd6b89765e5715e169481"
      "602d9c3d00000000000000000000000000000000000000000000000000000000"

      /************************** swapData[1] *************************/
      // callTo
      "000000000000000000000000c0788a3ad43d79aa53b09c2eacc313a787d1d607"
      // approveTo
      "000000000000000000000000c0788a3ad43d79aa53b09c2eacc313a787d1d607"
      // sendingAssetId
      "0000000000000000000000002791bca1f2de4661ed88a30c99a7a9449aa84174"
      // receivingAssetId
      "0000000000000000000000000000000000000000000000000000000000000000"
      // fromAmount
      "000000000000000000000000000000000000000000000000000000000007a310"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000000"

      "0000000000000000000000000000000000000000000000000000000000000104"
      "18cbafe500000000000000000000000000000000000000000000000000000000"
      "0007a310000000000000000000000000000000000000000000000000096eeba8"
      "455b6e3500000000000000000000000000000000000000000000000000000000"
      "000000a00000000000000000000000001231deb6f5749ef6ce6943a275a1d3e7"
      "486f4eae00000000000000000000000000000000000000000000000000000000"
      "66354c0400000000000000000000000000000000000000000000000000000000"
      "000000020000000000000000000000002791bca1f2de4661ed88a30c99a7a944"
      "9aa841740000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d"
      "3adf127000000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0x2791bca1f2de4661ed88a30c99a7a9449aa84174");  // USDC.e
  EXPECT_EQ(swap_info->source_amount, "0x7b451");           // 0.504913 USDC.e

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // MATIC
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "0x96eeba8455b6e35");  // 0.6797397017301765 MATIC

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);

  // Swap 1 MATIC → Y USDC.e
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x4630a0d8"  // function selector

      /***************************** HEAD ****************************/
      // transactionId
      "73bc2c896381e1296eefd4ddbbe7efbb62ae1d1968de6d764364d762f1fd9b9e"
      // integrator (offset)
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000100"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmountOut
      "0000000000000000000000000000000000000000000000000000000000098647"

      // offset to start data part of swapData
      "0000000000000000000000000000000000000000000000000000000000000160"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000"

      // offset to start data part of referrer
      // size of referrer string
      "000000000000000000000000000000000000000000000000000000000000002a"
      "3078303030303030303030303030303030303030303030303030303030303030"
      "3030303030303030303000000000000000000000000000000000000000000000"

      // size(swapData) = 1
      "0000000000000000000000000000000000000000000000000000000000000001"
      // swapData[0] offset
      "0000000000000000000000000000000000000000000000000000000000000020"

      /************************** swapData[0] *************************/
      // callTo
      "000000000000000000000000a5e0829caced8ffdd4de3c43696c57f7d7a678ff"
      // approveTo
      "000000000000000000000000a5e0829caced8ffdd4de3c43696c57f7d7a678ff"
      // sendingAssetId
      "0000000000000000000000000000000000000000000000000000000000000000"
      // receivingAssetId
      "0000000000000000000000002791bca1f2de4661ed88a30c99a7a9449aa84174"
      // fromAmount
      "0000000000000000000000000000000000000000000000000de0b6b3a7640000"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000001"

      "00000000000000000000000000000000000000000000000000000000000000e4"
      "7ff36ab500000000000000000000000000000000000000000000000000000000"
      "0009864700000000000000000000000000000000000000000000000000000000"
      "000000800000000000000000000000001231deb6f5749ef6ce6943a275a1d3e7"
      "486f4eae00000000000000000000000000000000000000000000000000000000"
      "6668bc8800000000000000000000000000000000000000000000000000000000"
      "000000020000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d"
      "3adf12700000000000000000000000002791bca1f2de4661ed88a30c99a7a944"
      "9aa8417400000000000000000000000000000000000000000000000000000000",
      &data));

  tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");   // MATIC
  EXPECT_EQ(swap_info->source_amount, "0xde0b6b3a7640000");  // 1 MATIC

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0x2791bca1f2de4661ed88a30c99a7a9449aa84174");  // USDC.e
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min, "0x98647");  // 0.624199 USDC.e

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);

  // zkSync Era has its own Diamond address.
  ExpectSwap(mojom::kZkSyncEraChainId, kLiFiDiamondZkSync, data);
  ExpectNotSwap(mojom::kZkSyncEraChainId, kLiFiDiamond, data);
  ExpectNotSwap(mojom::kMainnetChainId, kLiFiDiamondZkSync, data);

  ExpectNotSwap(mojom::kMainnetChainId, kNotASwapRouter, data);
  ExpectNotSwap(mojom::kMainnetChainId, kSquidRouter, data);
}

TEST(EthDataParser,
     GetTransactionInfoFromDataLiFiSwapTokensSingleNativeToERC20) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // TXN: ETH → token
  // Function:
  // swapTokensSingleNativeToERC20(bytes32 transactionId,
  //                               string integrator,
  //                               string referrer,
  //                               address receiver,
  //                               uint256 minAmountOut,
  //                               (address callTo,
  //                                address approveTo,
  //                                address sendingAssetId,
  //                                address receivingAssetId,
  //                                uint256 fromAmount,
  //                                bytes callData,
  //                                bool requiresDeposit) swapData)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x8f0af374"  // function selector

      /***************************** HEAD ****************************/
      // transactionId
      "94e4981b008883f7f7f34300c08227725cb259e7f341dfa94b81e2ed673b6571"
      // integrator (offset)
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000100"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmountOut
      "0000000000000000000000000000000000000000000000000000000000052397"

      // offset to start data part of swapData
      "0000000000000000000000000000000000000000000000000000000000000160"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000"

      // offset to start data part of referrer
      // size of referrer string
      "000000000000000000000000000000000000000000000000000000000000002a"
      // referrer string
      "3078303030303030303030303030303030303030303030303030303030303030"
      "3030303030303030303000000000000000000000000000000000000000000000"

      // callTo
      "0000000000000000000000006a000f20005980200259b80c5102003040001068"
      // approveTo
      "0000000000000000000000006a000f20005980200259b80c5102003040001068"
      // sendingAssetId
      "0000000000000000000000000000000000000000000000000000000000000000"
      // receivingAssetId
      "000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"
      // fromAmount
      "00000000000000000000000000000000000000000000000000005af3107a4000"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000001"

      "00000000000000000000000000000000000000000000000000000000000001e4"
      "e8bb3b6c00000000000000000000000000000000000000000000000000000000"
      "000000608c208b7b5625d78deb49240ef28126cbe27380981000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "000001c0000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      "eeeeeeee000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce"
      "3606eb4800000000000000000000000000000000000000000000000000005af3"
      "107a400000000000000000000000000000000000000000000000000000000000"
      "0005239700000000000000000000000000000000000000000000000000000000"
      "00052a3362ce29e87edc4049a22351ea61154ecf000000000000000000000000"
      "0133e4a600000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000010000000000000000000000000000000000000000000000000000000000"
      "00000040a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48c02aaa39b223fe8d"
      "0a0e5c4f27ead9083c756cc20000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // ETH
  EXPECT_EQ(swap_info->source_amount, "0x5af3107a4000");    // 0.0001 ETH

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");  // USDC
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min, "0x52397");  // 0.336791 USDC

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);
}

TEST(EthDataParser,
     GetTransactionInfoFromDataLiFiSwapTokensSingleERC20ToNative) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // TXN: token → ETH
  // Function:
  // swapTokensSingleERC20ToNative(bytes32 transactionId,
  //                               string integrator,
  //                               string referrer,
  //                               address receiver,
  //                               uint256 minAmountOut,
  //                               (address callTo,
  //                                address approveTo,
  //                                address sendingAssetId,
  //                                address receivingAssetId,
  //                                uint256 fromAmount,
  //                                bytes callData,
  //                                bool requiresDeposit) swapData)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xd5bc7be1"  // function selector

      /***************************** HEAD ****************************/
      // transactionId
      "68ec9ff6350e2e873779da24892008b7347f3b38f78150ab2a6f5ae21a211077"
      // integrator (offset)
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000100"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmountOut
      "000000000000000000000000000000000000000000000000000b4fb6da8128d1"

      // offset to start data part of swapData
      "0000000000000000000000000000000000000000000000000000000000000160"

      // offset to start data part of integrator
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000"

      // offset to start data part of referrer
      // size of referrer string
      "000000000000000000000000000000000000000000000000000000000000002a"
      // referrer string
      "3078303030303030303030303030303030303030303030303030303030303030"
      "3030303030303030303000000000000000000000000000000000000000000000"

      // callTo
      "000000000000000000000000e43ca1dee3f0fc1e2df73a0745674545f11a59f5"
      // approveTo
      "000000000000000000000000e43ca1dee3f0fc1e2df73a0745674545f11a59f5"
      // sendingAssetId
      "0000000000000000000000006b175474e89094c44da98b954eedeac495271d0f"
      // receivingAssetId
      "0000000000000000000000000000000000000000000000000000000000000000"
      // fromAmount
      "00000000000000000000000000000000000000000000000096324f4223190000"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000001"

      "0000000000000000000000000000000000000000000000000000000000000164"
      "2646478b0000000000000000000000006b175474e89094c44da98b954eedeac4"
      "95271d0f00000000000000000000000000000000000000000000000096324f42"
      "23190000000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      "eeeeeeee000000000000000000000000000000000000000000000000000b4fb6"
      "da8128d10000000000000000000000001231deb6f5749ef6ce6943a275a1d3e7"
      "486f4eae00000000000000000000000000000000000000000000000000000000"
      "000000c000000000000000000000000000000000000000000000000000000000"
      "00000073026b175474e89094c44da98b954eedeac495271d0f01ffff00a478c2"
      "975ab1ea89e8196811f51a7b7ade33eb1101e43ca1dee3f0fc1e2df73a074567"
      "4545f11a59f5000bb801c02aaa39b223fe8d0a0e5c4f27ead9083c756cc201ff"
      "ff02001231deb6f5749ef6ce6943a275a1d3e7486f4eae000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");    // DAI
  EXPECT_EQ(swap_info->source_amount, "0x96324f4223190000");  // 10.8228 DAI

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // ETH
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "0xb4fb6da8128d1");  // 0.003183871512357073 ETH

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);
}

TEST(EthDataParser,
     GetTransactionInfoFromDataLiFiSwapTokensSingleERC20ToERC20) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // TXN: token → token
  // Function:
  // swapTokensSingleERC20ToERC20(bytes32 transactionId,
  //                               string integrator,
  //                               string referrer,
  //                               address receiver,
  //                               uint256 minAmountOut,
  //                               (address callTo,
  //                                address approveTo,
  //                                address sendingAssetId,
  //                                address receivingAssetId,
  //                                uint256 fromAmount,
  //                                bytes callData,
  //                                bool requiresDeposit) swapData)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x878863a4"  // function selector

      /***************************** HEAD ****************************/
      // transactionId
      "53c2133b1535c6e5b73787aef9e24785aab9ef18bd4232ded93bafb34c351148"
      // integrator (offset)
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000100"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmountOut
      "0000000000000000000000000000000000000000000000061dc2169221089f5c"

      // offset to start data part of swapData
      "0000000000000000000000000000000000000000000000000000000000000160"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000"

      // offset to start data part of referrer
      // size of referrer string
      "000000000000000000000000000000000000000000000000000000000000002a"
      // referrer string
      "3078303030303030303030303030303030303030303030303030303030303030"
      "3030303030303030303000000000000000000000000000000000000000000000"

      // callTo
      "000000000000000000000000e43ca1dee3f0fc1e2df73a0745674545f11a59f5"
      // approveTo
      "000000000000000000000000e43ca1dee3f0fc1e2df73a0745674545f11a59f5"
      // sendingAssetId
      "0000000000000000000000006b175474e89094c44da98b954eedeac495271d0f"
      // receivingAssetId
      "0000000000000000000000000d8775f648430679a709e98d2b0cb6250d2887ef"
      // fromAmount
      "0000000000000000000000000000000000000000000000012c64655a698c7e2b"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000001"

      "0000000000000000000000000000000000000000000000000000000000000184"
      "2646478b0000000000000000000000006b175474e89094c44da98b954eedeac4"
      "95271d0f0000000000000000000000000000000000000000000000012c64655a"
      "698c7e2b0000000000000000000000000d8775f648430679a709e98d2b0cb625"
      "0d2887ef0000000000000000000000000000000000000000000000061dc21692"
      "21089f5c0000000000000000000000001231deb6f5749ef6ce6943a275a1d3e7"
      "486f4eae00000000000000000000000000000000000000000000000000000000"
      "000000c000000000000000000000000000000000000000000000000000000000"
      "00000087026b175474e89094c44da98b954eedeac495271d0f01ffff00a478c2"
      "975ab1ea89e8196811f51a7b7ade33eb1101b6909b960dbbe7392d405429eb2b"
      "3649752b4838000bb804c02aaa39b223fe8d0a0e5c4f27ead9083c756cc200b6"
      "909b960dbbe7392d405429eb2b3649752b4838001231deb6f5749ef6ce6943a2"
      "75a1d3e7486f4eae000bb8000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");  // DAI
  EXPECT_EQ(swap_info->source_amount,
            "0x12c64655a698c7e2b");  // 21.645537148041726 DAI

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0x0d8775f648430679a709e98d2b0cb6250d2887ef");  // BAT
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "0x61dc2169221089f5c");  // 112.82476563171433 BAT

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);
}

TEST(
    EthDataParser,
    GetTransactionInfoFromDataLiFiSwapAndStartBridgeTokensViaCelerCircleBridge) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // swapAndStartBridgeTokensViaCelerCircleBridge((bytes32 transactionId,
  //                                               string bridge,
  //                                               string integrator,
  //                                               string referrer,
  //                                               address sendingAssetId,
  //                                               address receiver,
  //                                               uint256 minAmount,
  //                                               uint256 destinationChainId,
  //                                               bool hasSourceSwaps,
  //                                               bool hasDestinationCalls)
  //                                               bridgeData),
  //                                              (address callTo,
  //                                               address approveTo,
  //                                               address sendingAssetId,
  //                                               address receivingAssetId,
  //                                               uint256 fromAmount,
  //                                               bytes callData,
  //                                               bool requiresDeposit)[]
  //                                               swapData)

  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x8fab0663"  // function selector

      /***************************** HEAD ****************************/
      // offset to start data part of bridgeData
      "0000000000000000000000000000000000000000000000000000000000000040"
      // offset to start data part of swapData
      "0000000000000000000000000000000000000000000000000000000000000200"

      /************************** bridgeData *************************/
      // transactionId
      "3c3e3d8fed4c117a7f0491b597137f1951b94dde6079b9d1d9c810b881493373"
      // bridge (offset)
      "0000000000000000000000000000000000000000000000000000000000000140"
      // integrator (offset)
      "0000000000000000000000000000000000000000000000000000000000000180"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000000"
      // sendingAssetId
      "0000000000000000000000003c499c542cef5e3811e1192ce70d8cc03d5c3359"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmount
      "0000000000000000000000000000000000000000000000000000000000098269"
      // destinationChainId
      "000000000000000000000000000000000000000000000000000000000000000a"
      // hasSourceSwaps
      "0000000000000000000000000000000000000000000000000000000000000001"
      // hasDestinationCalls
      "0000000000000000000000000000000000000000000000000000000000000000"

      // offset to start data part of bridge
      // size of bridge string
      "000000000000000000000000000000000000000000000000000000000000000b"
      // bridge string
      "63656c6572636972636c65000000000000000000000000000000000000000000"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000"

      // size(swapData) = 1
      "0000000000000000000000000000000000000000000000000000000000000001"
      // swapData[0] offset
      "0000000000000000000000000000000000000000000000000000000000000020"

      /************************** swapData[0] *************************/
      // callTo
      "00000000000000000000000039e3e49c99834c9573c9fc7ff5a4b226cd7b0e63"
      // approveTo
      "0000000000000000000000006d310348d5c12009854dfcf72e0df9027e8cb4f4"
      // sendingAssetId
      "0000000000000000000000002791bca1f2de4661ed88a30c99a7a9449aa84174"
      // receivingAssetId
      "0000000000000000000000003c499c542cef5e3811e1192ce70d8cc03d5c3359"
      // fromAmount
      "00000000000000000000000000000000000000000000000000000000000989b3"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000001"

      // extraneous calldata to be ignored
      "00000000000000000000000000000000000000000000000000000000000005c4"
      "301a37200000000000000000000000002791bca1f2de4661ed88a30c99a7a944"
      "9aa841740000000000000000000000003c499c542cef5e3811e1192ce70d8cc0"
      "3d5c335900000000000000000000000000000000000000000000000000000000"
      "000989b300000000000000000000000000000000000000000000000000000000"
      "0009826900000000000000000000000000000000000000000000000000000000"
      "0000016000000000000000000000000000000000000000000000000000000000"
      "000001e000000000000000000000000000000000000000000000000000000000"
      "0000026000000000000000000000000000000000000000000000000000000000"
      "0000000100000000000000000000000000000000000000000000000000000000"
      "0000030000000000000000000000000000000000000000000000000000000000"
      "0000056000000000000000000000000000000000000000000000000000000000"
      "66690f2500000000000000000000000000000000000000000000000000000000"
      "0000000300000000000000000000000026a0225e27227fa6b5ead5b029532ec5"
      "99c0a02e00000000000000000000000026a0225e27227fa6b5ead5b029532ec5"
      "99c0a02e0000000000000000000000006e7c5a418dd1395068e539514b8c4dc3"
      "98a12e9e00000000000000000000000000000000000000000000000000000000"
      "00000003000000000000000000000000ba12222222228d8ba445958a75a0704d"
      "566bf2c8000000000000000000000000ba12222222228d8ba445958a75a0704d"
      "566bf2c800000000000000000000000032fae204835e08b9374493d6b4628fd1"
      "f87dd04500000000000000000000000000000000000000000000000000000000"
      "0000000400000000000000000000000026a0225e27227fa6b5ead5b029532ec5"
      "99c0a02e00000000000000000000000026a0225e27227fa6b5ead5b029532ec5"
      "99c0a02e0000000000000000000000006e7c5a418dd1395068e539514b8c4dc3"
      "98a12e9e00000000000000000000000039e3e49c99834c9573c9fc7ff5a4b226"
      "cd7b0e6300000000000000000000000000000000000000000000000000000000"
      "0000000300000000000000000000000000000000000000000000000000000000"
      "0000006000000000000000000000000000000000000000000000000000000000"
      "000000e000000000000000000000000000000000000000000000000000000000"
      "0000016000000000000000000000000000000000000000000000000000000000"
      "00000060d208168d2a512240eb82582205d94a0710bce4e70001000000000000"
      "000000380000000000000000000000002791bca1f2de4661ed88a30c99a7a944"
      "9aa841740000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d"
      "3adf127000000000000000000000000000000000000000000000000000000000"
      "00000060aa56a0854d98c3c1e32d624d5a8eb60cc090249c0001000000000000"
      "000007cd0000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d"
      "3adf12700000000000000000000000001bfd67037b42cf73acf2047067bd4f2c"
      "47d9bfd600000000000000000000000000000000000000000000000000000000"
      "000000c000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000004000000000000000000000000000000000000000000000000000000000"
      "000000600000000000000000000000001bfd67037b42cf73acf2047067bd4f2c"
      "47d9bfd60000000000000000000000003c499c542cef5e3811e1192ce70d8cc0"
      "3d5c335900000000000000000000000000000000000000000000000000000000"
      "000001f400000000000000000000000000000000000000000000000000000000"
      "0000004000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0x2791bca1f2de4661ed88a30c99a7a9449aa84174");  // USDC.e
  EXPECT_EQ(swap_info->source_amount, "0x989b3");           // 0.625075 USDC.e

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "0xa");
  EXPECT_EQ(swap_info->destination_token_address,
            "");  // cannot be reliably determined from the data
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "");  // cannot be reliably determined from the data

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);
}

TEST(EthDataParser,
     GetTransactionInfoFromDataLiFiSwapAndStartBridgeTokensViaAmarok) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // swapAndStartBridgeTokensViaAmarok((bytes32 transactionId,
  //                                    string bridge,
  //                                    string integrator,
  //                                    string referrer,
  //                                    address sendingAssetId,
  //                                    address receiver,
  //                                    uint256 minAmount,
  //                                    uint256 destinationChainId,
  //                                    bool hasSourceSwaps,
  //                                    bool hasDestinationCalls) bridgeData),
  //                                   (address callTo,
  //                                    address approveTo,
  //                                    address sendingAssetId,
  //                                    address receivingAssetId,
  //                                    uint256 fromAmount,
  //                                    bytes callData,
  //                                    bool requiresDeposit)[] swapData)

  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x83f31917"  // function selector

      /***************************** HEAD ****************************/
      // offset to start data part of bridgeData
      "0000000000000000000000000000000000000000000000000000000000000060"
      // offset to start data part of swapData
      "0000000000000000000000000000000000000000000000000000000000000220"
      // offset to start data part of bridge specific data (ignored)
      "0000000000000000000000000000000000000000000000000000000000000860"

      /************************** bridgeData *************************/
      // transactionId
      "51ab364e57b0852ef6ac1b026e0062a5ded055416bc91a8a3de1dd8a1bc61bd2"
      // bridge (offset)
      "0000000000000000000000000000000000000000000000000000000000000140"
      // integrator (offset)
      "0000000000000000000000000000000000000000000000000000000000000180"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000000"
      // sendingAssetId
      "0000000000000000000000002170ed0880ac9a755fd29b2688956bd959f933f8"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmount
      "000000000000000000000000000000000000000000000000000024e83b6de5bb"
      // destinationChainId
      "000000000000000000000000000000000000000000000000000000000000000a"
      // hasSourceSwaps
      "0000000000000000000000000000000000000000000000000000000000000001"
      // hasDestinationCalls
      "0000000000000000000000000000000000000000000000000000000000000001"

      // offset to start data part of bridge
      // size of bridge string
      "0000000000000000000000000000000000000000000000000000000000000006"
      // bridge string
      "616d61726f6b0000000000000000000000000000000000000000000000000000"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      "6272617665000000000000000000000000000000000000000000000000000000"

      // size(swapData) = 1
      "0000000000000000000000000000000000000000000000000000000000000001"
      // swapData[0] offset
      "0000000000000000000000000000000000000000000000000000000000000020"

      /************************** swapData[0] *************************/
      // callTo
      "0000000000000000000000000656fd85364d03b103ceeda192fb2d3906a6ac15"
      // approveTo
      "000000000000000000000000a128ba44b2738a558a1fdc06d6303d52d3cef8c1"
      // sendingAssetId
      "000000000000000000000000ad29abb318791d579433d831ed122afeaf29dcfe"
      // receivingAssetId
      "0000000000000000000000002170ed0880ac9a755fd29b2688956bd959f933f8"
      // fromAmount
      "0000000000000000000000000000000000000000000000000318632acb5ec3c1"
      // callData
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // requiresDeposit
      "0000000000000000000000000000000000000000000000000000000000000001"

      // extraneous calldata to be ignored (truncated)

      "00000000000000000000000000000000000000000000000000000000000004e4"
      "301a3720000000000000000000000000ad29abb318791d579433d831ed122afe"
      "af29dcfe0000000000000000000000002170ed0880ac9a755fd29b2688956bd9"
      "59f933f80000000000000000000000000000000000000000000000000318632a"
      "cb5ec3c1000000000000000000000000000000000000000000000000000024e8"
      "3b70dd6300000000000000000000000000000000000000000000000000000000"
      "0000016000000000000000000000000000000000000000000000000000000000"
      "000001e000000000000000000000000000000000000000000000000000000000"
      "0000026000000000000000000000000000000000000000000000000000000000"
      "0000000600000000000000000000000000000000000000000000000000000000"
      "0000030000000000000000000000000000000000000000000000000000000000"
      "0000048000000000000000000000000000000000000000000000000000000000"
      "6686f06f00000000000000000000000000000000000000000000000000000000"
      "000000030000000000000000000000008e95bce2a39eb72f231f301516441840"
      "423bbb98000000000000000000000000165ba87e882208100672b6c56f477ee4"
      "2502c8200000000000000000000000004c52f61212f9e36922ab78aab250f1a2"
      "f000d93c00000000000000000000000000000000000000000000000000000000"
      "00000003000000000000000000000000f753b7e59e1c010b5fd89737e874d4ca"
      "51d9cc230000000000000000000000000f36544d0b1a107b98edfabb1d95538c"
      "316c1dcd000000000000000000000000d9a0d1f5e02de2403f68bb71a15f8847"
      "a854b49400000000000000000000000000000000000000000000000000000000"
      "00000004000000000000000000000000f753b7e59e1c010b5fd89737e874d4ca"
      "51d9cc230000000000000000000000000f36544d0b1a107b98edfabb1d95538c"
      "316c1dcd000000000000000000000000d9a0d1f5e02de2403f68bb71a15f8847"
      "a854b4940000000000000000000000000656fd85364d03b103ceeda192fb2d39"
      "06a6ac1500000000000000000000000000000000000000000000000000000000"
      "0000000300000000000000000000000000000000000000000000000000000000"
      "0000006000000000000000000000000000000000000000000000000000000000"
      "000000c000000000000000000000000000000000000000000000000000000000"
      "0000010000000000000000000000000000000000000000000000000000000000"
      "00000040000000000000000000000000ad29abb318791d579433d831ed122afe"
      "af29dcfe000000000000000000000000bb4cdb9cbd36b01bd1cbaebf2de08d91"
      "73bc095c00000000000000000000000000000000000000000000000000000000"
      "0000000100000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000004000000000000000000000000000000000000000000000000000000000"
      "0000001e00000000000000000000000000000000000000000000000000000000"
      "0000271000000000000000000000000000000000000000000000000000000000"
      "0000004000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      "000000000000000000000000050e198e36a73a1e32f15c3afc58c4506d82f657"
      "0000000000000000000000000000000000000000000000000002d2c1c1a8bbbe"
      "0000000000000000000000000000000000000000000000000000000000000032"
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      "000000000000000000000000000000000000000000000000000000006f707469"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000001a0"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000005215e9fd223bc909083fbdb2860213873046e45d"
      "0000000000000000000000005215e9fd223bc909083fbdb2860213873046e45d"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000002484aa1eb76b"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000004"
      "3ccfd60b00000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xad29abb318791d579433d831ed122afeaf29dcfe");  // FTM
  EXPECT_EQ(swap_info->source_amount,
            "0x318632acb5ec3c1");  // 0.223037217006601150 FTM

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "0xa");  // Optimism
  EXPECT_EQ(swap_info->destination_token_address,
            "");  // cannot be reliably determined from the data
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "");  // cannot be reliably determined from the data

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);
}

TEST(EthDataParser,
     GetTransactionInfoFromDataLiFiStartBridgeTokensViaCelerCircleBridge) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // startBridgeTokensViaCelerCircleBridge((bytes32 transactionId,
  //                                        string bridge,
  //                                        string integrator,
  //                                        string referrer,
  //                                        address sendingAssetId,
  //                                        address receiver,
  //                                        uint256 minAmount,
  //                                        uint256 destinationChainId,
  //                                        bool hasSourceSwaps,
  //                                        bool hasDestinationCalls)
  //                                        bridgeData)

  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xbab657d8"  // function selector

      /***************************** HEAD ****************************/
      // offset to start data part of bridgeData
      "0000000000000000000000000000000000000000000000000000000000000020"

      /************************** bridgeData *************************/
      // transactionId
      "098ba17b1ad0ba045e8da5786487a91e0f9ce74fb858715fc3cab5e554f47cef"

      // bridge (offset)
      "0000000000000000000000000000000000000000000000000000000000000140"
      // integrator (offset)
      "0000000000000000000000000000000000000000000000000000000000000180"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000000"
      // sendingAssetId
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      // receiver
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // minAmount
      "00000000000000000000000000000000000000000000000000000000001e8480"
      // destinationChainId
      "000000000000000000000000000000000000000000000000000000000000a4b1"
      // hasSourceSwaps
      "0000000000000000000000000000000000000000000000000000000000000000"
      // hasDestinationCalls
      "0000000000000000000000000000000000000000000000000000000000000000"

      // offset to start data part of bridge
      // size of bridge string
      "000000000000000000000000000000000000000000000000000000000000000b"
      // bridge string
      "63656c6572636972636c65000000000000000000000000000000000000000000"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0x0b2c639c533813f4aa9d7837caf62653d097ff85");  // USDC
  EXPECT_EQ(swap_info->source_amount, "0x1e8480");          // 0.2 USDC

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "0xa4b1");  // Arbitrum
  EXPECT_EQ(swap_info->destination_token_address, "");
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min, "");

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);
}

TEST(EthDataParser, GetTransactionInfoFromDataLiFiStartBridgeTokensViaMayan) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // startBridgeTokensViaMayan((bytes32 transactionId,
  //                            string bridge,
  //                            string integrator,
  //                            string referrer,
  //                            address sendingAssetId,
  //                            address receiver,
  //                            uint256 minAmount,
  //                            uint256 destinationChainId,
  //                            bool hasSourceSwaps,
  //                            bool hasDestinationCalls)
  //                            bridgeData)

  // Bridge BNB on BSC -> USDC on Solana
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0xb621b032"  // function selector

      /***************************** HEAD ****************************/
      // offset to start data part of bridgeData
      "0000000000000000000000000000000000000000000000000000000000000040"

      // offset to start of extraneous data (ignored)
      "0000000000000000000000000000000000000000000000000000000000000200"

      /************************** bridgeData *************************/

      // transactionId
      "0e83f6be858a49d81d2972bd954c73f5ca8591de4cbdb94dce6d0d47d39ba16e"

      // bridge (offset)
      "0000000000000000000000000000000000000000000000000000000000000140"
      // integrator (offset)
      "0000000000000000000000000000000000000000000000000000000000000180"
      // referrer
      "0000000000000000000000000000000000000000000000000000000000000000"
      // sendingAssetId
      "0000000000000000000000000000000000000000000000000000000000000000"
      // receiver
      "00000000000000000000000011f111f111f111f111f111f111f111f111f111f1"
      // minAmount
      "00000000000000000000000000000000000000000000000001753753f24cb000"
      // destinationChainId
      "000000000000000000000000000000000000000000000000000416edef1601be"
      // hasSourceSwaps
      "0000000000000000000000000000000000000000000000000000000000000000"
      // hasDestinationCalls
      "0000000000000000000000000000000000000000000000000000000000000000"

      // offset to start data part of bridge
      // size of bridge string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // bridge string
      "6d6179616e000000000000000000000000000000000000000000000000000000"

      // offset to start data part of integrator
      // size of integrator string
      "0000000000000000000000000000000000000000000000000000000000000005"
      // integrator string
      "6272617665000000000000000000000000000000000000000000000000000000"

      // extraneous calldata (ignored)
      "71a6faa032fb1e7bd69b0811c2bd0199173bc2b6f3e6a96af79025d09f256b78"
      "000000000000000000000000bf5f3f65102ae745a48bd521d10bab5bf02a9ef4"
      "0000000000000000000000000000000000000000000000000000000000000060"
      "0000000000000000000000000000000000000000000000000000000000000284"
      "1eb1cff000000000000000000000000000000000000000000000000000000000"
      "0008201300000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000b3b0d0dc812e5333ead0d99ab5d2fe0929c343ba58d58abd0d8a78459a6e"
      "9edda26d00000000000000000000000000000000000000000000000000000000"
      "000000016dfa43f824c3b8b61e715fe8bf447f2aba63e59ab537f186cf665152"
      "c2114c3971a6faa032fb1e7bd69b0811c2bd0199173bc2b6f3e6a96af79025d0"
      "9f256b7800000000000000000000000000000000000000000000000000000000"
      "000000011e8c4fab8994494c8f1e5c1287445b2917d60c43c79aa959162f5d60"
      "00598d32000000000000000000000000a92d461a9a988a7f11ec285d39783a63"
      "7fdd6ba4c6fa7af3bedbad3a3d65f36aabc97431b1bbe4c2d2f6e0e47ca60203"
      "452f5d6100000000000000000000000000000000000000000000000000000000"
      "0000000100000000000000000000000000000000000000000000000000000000"
      "000001a000000000000000000000000000000000000000000000000000000000"
      "66b3884000000000000000000000000000000000000000000000000000000000"
      "66b3884000000000000000000000000000000000000000000000000000000000"
      "02d6552400000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "000000c000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kLiFiDiamond, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");   // BNB
  EXPECT_EQ(swap_info->source_amount, "0x1753753f24cb000");  // 0.105051 BNB

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::SOL);
  EXPECT_EQ(swap_info->destination_chain_id, mojom::kSolanaMainnet);
  EXPECT_EQ(swap_info->destination_token_address,
            "");  // cannot be reliably determined from the data
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "");  // cannot be reliably determined from the data

  EXPECT_EQ(swap_info->recipient,
            "");  // cannot be reliably determined from the data
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kLiFi);
}

TEST(EthDataParser,
     GetTransactionInfoFromDataSquidFundAndRunMulticallWithExactInputSingle) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // fundAndRunMulticall(address token,
  //                     uint256 amount,
  //                     (bytes32 callType,
  //                      address target,
  //                      uint256 value,
  //                      bytes callData,
  //                      bytes payload)[] calls)

  // Swap ETH -> USDC.e on Optimism
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // function selector
      "0x58181a80"
      // token
      "000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      // amount
      "00000000000000000000000000000000000000000000000000005af3107a4000"

      // offset to start data part of calls
      "0000000000000000000000000000000000000000000000000000000000000060"
      // size(calls) = 5
      "0000000000000000000000000000000000000000000000000000000000000005"
      // calls[0] offset = 160 = 5 words
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // calls[1] offset = 480 = 15 words
      "00000000000000000000000000000000000000000000000000000000000001e0"
      // calls[2] offset = 864 = 27 words
      "0000000000000000000000000000000000000000000000000000000000000360"
      // calls[3] offset = 1408 = 44 words
      "0000000000000000000000000000000000000000000000000000000000000580"
      // calls[4] offset = 1792 = 56 words
      "0000000000000000000000000000000000000000000000000000000000000700"

      /*************************** calls[0] ***************************/
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "00000000000000000000000000000000000000000000000000005af3107a4000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      "0000000000000000000000000000000000000000000000000000000000000004"
      "d0e30db000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /*************************** calls[1] ***************************/
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000120"
      "0000000000000000000000000000000000000000000000000000000000000044"
      "095ea7b300000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd"
      "8665fc45ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "ffffffff00000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[2] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "00000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd8665fc45"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData = 5 words
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload = 14 words
      "00000000000000000000000000000000000000000000000000000000000001c0"

      // callData
      // size(callData) = 228
      "00000000000000000000000000000000000000000000000000000000000000e4"

      "04e45aaf"  // exactInputSingle function selector
      // callData.tokenIn
      "0000000000000000000000004200000000000000000000000000000000000006"
      // callData.tokenOut
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      // callData.fee
      "00000000000000000000000000000000000000000000000000000000000001f4"
      // callData.recipient
      "000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769229b896c"
      // callData.amountIn
      "00000000000000000000000000000000000000000000000000005af3107a4000"
      // callData.amountOutMinimum
      "00000000000000000000000000000000000000000000000000000000000389e2"
      // callData.sqrtPriceLimitX96
      "0000000000000000000000000000000000000000000000000000000000000000"
      // padding of (32 - 4) bytes for exactInputSingle function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000004"

      /*************************** calls[3] ***************************/
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000120"
      "0000000000000000000000000000000000000000000000000000000000000044"
      "095ea7b300000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd"
      "8665fc45ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "ffffffff00000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[4] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "00000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd8665fc45"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData = 5 words
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload = 14 words
      "00000000000000000000000000000000000000000000000000000000000001c0"

      // callData
      // size(callData) = 228
      "00000000000000000000000000000000000000000000000000000000000000e4"

      "04e45aaf"  // exactInputSingle function selector
      // callData.tokenIn
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      // callData.tokenOut
      "0000000000000000000000007f5c764cbc14f9669b88837ca1490cca17c31607"
      // callData.fee
      "0000000000000000000000000000000000000000000000000000000000000064"
      // callData.recipient
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // callData.amountIn
      "0000000000000000000000000000000000000000000000000000000000038c28"
      // callData.amountOutMinimum
      "00000000000000000000000000000000000000000000000000000000000389d6"
      // callData.sqrtPriceLimitX96
      "0000000000000000000000000000000000000000000000000000000000000000"
      // padding of (32 - 4) bytes for exactInputSingle function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      "0000000000000000000000000000000000000000000000000000000000000004"

      // extraneous calldata (ignored)
      "e8d8c74b6eb85b5c0c7de1143534b4af",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kSquidRouter, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // ETH
  EXPECT_EQ(swap_info->source_amount, "0x5af3107a4000");    // 0.0001 ETH

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0x7f5c764cbc14f9669b88837ca1490cca17c31607");  // USDC.e
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min, "0x389d6");  // 0.231894 USDC.e

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kSquid);

  // The SquidRouter proxy has no implementation on Blast.
  ExpectNotSwap(mojom::kBlastMainnetChainId, kSquidRouter, data);

  ExpectNotSwap(mojom::kMainnetChainId, kNotASwapRouter, data);
  ExpectNotSwap(mojom::kMainnetChainId, kCowSwapEthFlow, data);
}

TEST(
    EthDataParser,
    GetTransactionInfoFromDataSquidFundAndRunMulticallWithSwapExactTokensForTokensSellEth) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // fundAndRunMulticall(address token,
  //                     uint256 amount,
  //                     (bytes32 callType,
  //                      address target,
  //                      uint256 value,
  //                      bytes callData,
  //                      bytes payload)[] calls)

  // Swap ETH -> USDC.e on Optimism
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // function selector
      "0x58181a80"
      // token
      "000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      // amount
      "000000000000000000000000000000000000000000000000000570efc5b4ac00"

      // offset to start data part of calls
      "0000000000000000000000000000000000000000000000000000000000000060"
      // size(calls) = 5
      "0000000000000000000000000000000000000000000000000000000000000005"
      // calls[0] offset = 160 = 5 words
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // calls[1] offset = 480 = 15 words
      "00000000000000000000000000000000000000000000000000000000000001e0"
      // calls[2] offset = 864 = 27 words
      "0000000000000000000000000000000000000000000000000000000000000360"
      // calls[3] offset = 1408 = 44 words
      "0000000000000000000000000000000000000000000000000000000000000580"
      // calls[4] offset = 1792 = 56 words
      "0000000000000000000000000000000000000000000000000000000000000700"

      /************************** calls[0-3] **************************/
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "000000000000000000000000000000000000000000000000000570efc5b4ac00"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      "0000000000000000000000000000000000000000000000000000000000000004"
      "d0e30db000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000120"
      "0000000000000000000000000000000000000000000000000000000000000044"
      "095ea7b300000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd"
      "8665fc45ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "ffffffff00000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd8665fc45"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000001c0"
      "00000000000000000000000000000000000000000000000000000000000000e4"
      "04e45aaf00000000000000000000000042000000000000000000000000000000"
      "000000060000000000000000000000000b2c639c533813f4aa9d7837caf62653"
      "d097ff8500000000000000000000000000000000000000000000000000000000"
      "000001f4000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769"
      "229b896c000000000000000000000000000000000000000000000000000570ef"
      "c5b4ac0000000000000000000000000000000000000000000000000000000000"
      "0038e3a100000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000004"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000120"
      "0000000000000000000000000000000000000000000000000000000000000044"
      "095ea7b3000000000000000000000000a062ae8a9c5e11aaa026fc2670b0d65c"
      "cc8b2858ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "ffffffff00000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[4] ***************************/

      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "000000000000000000000000a062ae8a9c5e11aaa026fc2670b0d65ccc8b2858"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData = 5 words
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload = 17 words
      "0000000000000000000000000000000000000000000000000000000000000220"

      // callData
      // size(callData) = 324 bytes
      "0000000000000000000000000000000000000000000000000000000000000144"

      "cac88ea9"  // swapExactTokensForTokens function selector
      // callData.amountIn
      "0000000000000000000000000000000000000000000000000000000000390822"
      // callData.amountOutMin
      "000000000000000000000000000000000000000000000000000000000038dfbc"

      // offset to start data part of callData.routes
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // callData.to
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // callData.deadline
      "00000000000000000000000000000000000000000000000000000192702460dd"

      // size(callData.routes) = 1
      "0000000000000000000000000000000000000000000000000000000000000001"
      // callData.routes[0].from
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      // callData.routes[0].to
      "0000000000000000000000007f5c764cbc14f9669b88837ca1490cca17c31607"
      // callData.routes[0].stable
      "0000000000000000000000000000000000000000000000000000000000000001"
      // callData.routes[0].factory
      "000000000000000000000000f1046053aa5682b4f9a81b5481394da16be5ff5a"

      // padding of (32 - 4) bytes for swapExactTokensForTokens function
      // selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64 bytes
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000b2c639c533813f4aa9d7837caf62653d097ff85"
      "0000000000000000000000000000000000000000000000000000000000000000"

      // extraneous calldata (ignored)
      "df041a3408ef8216821804ab8ebed608",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kSquidRouter, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // ETH
  EXPECT_EQ(swap_info->source_amount, "0x570efc5b4ac00");   // 0.00153155 ETH

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0x7f5c764cbc14f9669b88837ca1490cca17c31607");  // USDC.e
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min, "0x38dfbc");  // 3.727292 USDC.e

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kSquid);
}

TEST(
    EthDataParser,
    GetTransactionInfoFromDataSquidFundAndRunMulticallWithSwapExactTokensForTokensBuyEth) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // fundAndRunMulticall(address token,
  //                     uint256 amount,
  //                     (bytes32 callType,
  //                      address target,
  //                      uint256 value,
  //                      bytes callData,
  //                      bytes payload)[] calls)

  // Swap USDC.e -> ETH on Optimism
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // function selector
      "0x58181a80"
      // token
      "0000000000000000000000007f5c764cbc14f9669b88837ca1490cca17c31607"
      // amount
      "0000000000000000000000000000000000000000000000000000000000003c97"
      // offset to start data part of calls
      "0000000000000000000000000000000000000000000000000000000000000060"
      // size(calls) = 6
      "0000000000000000000000000000000000000000000000000000000000000006"
      // calls[0] offset = 192 = 6 words
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // calls[1] offset = 576 = 18 words
      "0000000000000000000000000000000000000000000000000000000000000240"
      // calls[2] offset = 1216 = 38 words
      "00000000000000000000000000000000000000000000000000000000000004c0"
      // calls[3] offset = 1600 = 50 words
      "0000000000000000000000000000000000000000000000000000000000000640"
      // calls[4] offset = 2240 = 70 words
      "00000000000000000000000000000000000000000000000000000000000008c0"
      // calls[5] offset = 2592 = 81 words
      "0000000000000000000000000000000000000000000000000000000000000a20"

      /*************************** calls[0] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "0000000000000000000000007f5c764cbc14f9669b88837ca1490cca17c31607"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000120"
      // callData size = 68
      "0000000000000000000000000000000000000000000000000000000000000044"
      "095ea7b3"  // function selector
      "000000000000000000000000a062ae8a9c5e11aaa026fc2670b0d65ccc8b2858"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000007f5c764cbc14f9669b88837ca1490cca17c31607"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[1] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "000000000000000000000000a062ae8a9c5e11aaa026fc2670b0d65ccc8b2858"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000220"
      // callData size = 324
      "0000000000000000000000000000000000000000000000000000000000000144"
      "cac88ea9"  // swapExactTokensForTokens function selector
      // callData.amountIn
      "0000000000000000000000000000000000000000000000000000000000003c97"
      // callData.amountOutMin
      "0000000000000000000000000000000000000000000000000021d31f1cf08a28"
      // offset to start data part of callData.routes
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // callData.to
      "000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769229b896c"
      // callData.deadline
      "00000000000000000000000000000000000000000000000000000192716158cd"
      // size(callData.routes) = 1
      "0000000000000000000000000000000000000000000000000000000000000001"
      // callData.routes[0].from
      "0000000000000000000000007f5c764cbc14f9669b88837ca1490cca17c31607"
      // callData.routes[0].to
      "0000000000000000000000004200000000000000000000000000000000000042"
      // callData.routes[0].stable
      "0000000000000000000000000000000000000000000000000000000000000000"
      // callData.routes[0].factory
      "000000000000000000000000f1046053aa5682b4f9a81b5481394da16be5ff5a"
      // padding of (32 - 4) bytes for swapExactTokensForTokens function
      // selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000007f5c764cbc14f9669b88837ca1490cca17c31607"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /*************************** calls[2] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "0000000000000000000000004200000000000000000000000000000000000042"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000120"
      // callData size = 68
      "0000000000000000000000000000000000000000000000000000000000000044"
      // function selector
      "095ea7b3"
      "000000000000000000000000a062ae8a9c5e11aaa026fc2670b0d65ccc8b2858"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000042"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[3] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "000000000000000000000000a062ae8a9c5e11aaa026fc2670b0d65ccc8b2858"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000220"
      // callData size = 324
      "0000000000000000000000000000000000000000000000000000000000000144"
      // swapExactTokensForTokens function selector
      "cac88ea9"
      // callData.amountIn
      "0000000000000000000000000000000000000000000000000021e8d2d713ca06"
      // callData.amountOutMin
      "000000000000000000000000000000000000000000000000000005c27c64134e"
      // offset to start data part of callData.routes
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // callData.to
      "000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769229b896c"
      // callData.deadline
      "00000000000000000000000000000000000000000000000000000192716158ce"
      // size(callData.routes) = 1
      "0000000000000000000000000000000000000000000000000000000000000001"
      // callData.routes[0].from
      "0000000000000000000000004200000000000000000000000000000000000042"
      // callData.routes[0].to
      "0000000000000000000000004200000000000000000000000000000000000006"
      // callData.routes[0].stable
      "0000000000000000000000000000000000000000000000000000000000000000"
      // callData.routes[0].factory
      "000000000000000000000000f1046053aa5682b4f9a81b5481394da16be5ff5a"
      // padding of (32 - 4) bytes for swapExactTokensForTokens function
      // selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000042"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /*************************** calls[4] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "0000000000000000000000004200000000000000000000000000000000000006"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000100"
      // callData size = 36
      "0000000000000000000000000000000000000000000000000000000000000024"
      // function selector
      "2e1a7d4d"
      "0000000000000000000000000000000000000000000000000000000000000000"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000004200000000000000000000000000000000000006"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /*************************** calls[5] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000002"
      // target
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // callData size = 0
      "0000000000000000000000000000000000000000000000000000000000000000"
      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      "0000000000000000000000000000000000000000000000000000000000000000"

      // extraneous calldata (ignored)
      "9f2297047c81ae9316f31dea71638a36",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kSquidRouter, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0x7f5c764cbc14f9669b88837ca1490cca17c31607");  // USDC.e
  EXPECT_EQ(swap_info->source_amount, "0x3c97");            // 0.15511 USDC.e

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // ETH
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "0x5c27c64134e");  // 0.00000633286872763 ETH

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kSquid);
}

TEST(EthDataParser, SwapExactTokensForTokensEmptyRoutesTest) {
  std::vector<uint8_t> data;

  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x58181a80"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000060"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000dead"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000001a0"
      "00000000000000000000000000000000000000000000000000000000000000c4"
      "cac88ea9"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "000000000000000000000000000000000000000000000000000000000000dead"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"  // 0
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000",
      &data));

  EXPECT_FALSE(GetTransactionInfoFromData(mojom::kMainnetChainId, kSquidRouter,
                                          data));  // no crash.
}

TEST(EthDataParser,
     GetTransactionInfoFromDataSquidFundAndRunMulticallWithExactInputSingleV2) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // fundAndRunMulticall(address token,
  //                     uint256 amount,
  //                     (bytes32 callType,
  //                      address target,
  //                      uint256 value,
  //                      bytes callData,
  //                      bytes payload)[] calls)

  // Swap ETH -> USDC on Arbitrum
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // function selector
      "0x58181a80"
      // token
      "000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      // amount
      "00000000000000000000000000000000000000000000000000038d7ea4c68000"
      // offset to start data part of calls
      "0000000000000000000000000000000000000000000000000000000000000060"
      // size(calls) = 3
      "0000000000000000000000000000000000000000000000000000000000000003"
      // calls[0] offset = 3 words
      "0000000000000000000000000000000000000000000000000000000000000060"
      // calls[1] offset = 18 words
      "00000000000000000000000000000000000000000000000000000000000001a0"
      // calls[2] offset = 25 words
      "0000000000000000000000000000000000000000000000000000000000000320"

      /*************************** calls[0] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "00000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1"
      // value
      "00000000000000000000000000000000000000000000000000038d7ea4c68000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // callData
      // size(callData) = 4
      "0000000000000000000000000000000000000000000000000000000000000004"
      // deposit() function selector
      "d0e30db0"
      // padding of (32 - 4) bytes for deposit() function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "00000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /*************************** calls[1] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "00000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000120"
      // callData
      // size(callData) = 68
      "0000000000000000000000000000000000000000000000000000000000000044"
      // function selector
      "095ea7b3"
      "0000000000000000000000001f721e2e82f6676fce4ea07a5958cf098d339e18"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "00000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[2] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "0000000000000000000000001f721e2e82f6676fce4ea07a5958cf098d339e18"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "00000000000000000000000000000000000000000000000000000000000001c0"
      // callData
      // size(callData) = 228
      "00000000000000000000000000000000000000000000000000000000000000e4"
      // exactInputSingle() function selector
      "bc651188"
      // callData.tokenIn
      "00000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1"
      // callData.tokenOut
      "000000000000000000000000af88d065e77c8cc2239327c5edb3a432268e5831"
      // callData.recipient
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      // callData.deadline
      "0000000000000000000000000000000000000000000000000000019277b6646d"
      // callData.amountIn
      "00000000000000000000000000000000000000000000000000038d7ea4c68000"
      // callData.amountOutMinimum
      "0000000000000000000000000000000000000000000000000000000000239039"
      // callData.sqrtPriceLimitX96
      "0000000000000000000000000000000000000000000000000000000000000000"
      // padding of (32 - 4) bytes for exactInputSingle() function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "00000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1"
      "0000000000000000000000000000000000000000000000000000000000000004"

      // extraneous calldata (ignored)
      "cc6400c8bcb752d810d096c08873ae48",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kSquidRouter, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // ETH
  EXPECT_EQ(swap_info->source_amount, "0x38d7ea4c68000");   // 0.0001 ETH

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0xaf88d065e77c8cc2239327c5edb3a432268e5831");  // USDC
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min, "0x239039");  // 2.330681 USDC

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kSquid);
}

TEST(EthDataParser,
     GetTransactionInfoFromDataSquidFundAndRunMulticallWithExchange) {
  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  mojom::SwapInfoPtr swap_info;
  std::vector<uint8_t> data;

  // Function:
  // fundAndRunMulticall(address token,
  //                     uint256 amount,
  //                     (bytes32 callType,
  //                      address target,
  //                      uint256 value,
  //                      bytes callData,
  //                      bytes payload)[] calls)

  // Swap ETH -> DAI on Ethereum
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // function selector
      "0x58181a80"
      // token
      "000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      // amount
      "000000000000000000000000000000000000000000000000000009184e72a000"
      // offset to start data part of calls
      "0000000000000000000000000000000000000000000000000000000000000060"
      // size(calls) = 6
      "0000000000000000000000000000000000000000000000000000000000000006"
      // calls[0] offset = 6 words
      "00000000000000000000000000000000000000000000000000000000000000c0"
      // calls[1] offset = 16 words
      "0000000000000000000000000000000000000000000000000000000000000200"
      // calls[2] offset = 28 words
      "0000000000000000000000000000000000000000000000000000000000000380"
      // calls[3] offset = 46 words
      "00000000000000000000000000000000000000000000000000000000000005c0"
      // calls[4] offset = 58 words
      "0000000000000000000000000000000000000000000000000000000000000740"
      // calls[5] offset = 72 words
      "0000000000000000000000000000000000000000000000000000000000000900"

      /*************************** calls[0] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
      // value
      "000000000000000000000000000000000000000000000000000009184e72a000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // callData
      // size(callData) = 4
      "0000000000000000000000000000000000000000000000000000000000000004"
      // deposit() function selector
      "d0e30db0"
      // padding of (32 - 4) bytes for deposit() function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /*************************** calls[1] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000120"
      // callData
      "0000000000000000000000000000000000000000000000000000000000000044"
      // function selector
      "095ea7b3"
      "0000000000000000000000007a250d5630b4cf539739df2c5dacb4c659f2488d"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[2] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "0000000000000000000000007a250d5630b4cf539739df2c5dacb4c659f2488d"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "00000000000000000000000000000000000000000000000000000000000001e0"
      // callData
      "0000000000000000000000000000000000000000000000000000000000000104"
      // function selector
      "38ed1739"
      "000000000000000000000000000000000000000000000000000009184e72a000"
      "0000000000000000000000000000000000000000000000000000000000005b6e"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769229b896c"
      "0000000000000000000000000000000000000000000000000000019277f94523"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
      "000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"
      "0000000000000000000000000000000000000000000000000000000000000000"

      /*************************** calls[3] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000000"
      // target
      "000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000120"
      // callData
      // size(callData) = 68
      "0000000000000000000000000000000000000000000000000000000000000044"
      // function selector
      "095ea7b3"
      "000000000000000000000000bebc44782c7db0a1a60cb6fe97d0b483032ff1c7"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"
      "0000000000000000000000000000000000000000000000000000000000000001"

      /*************************** calls[4] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "000000000000000000000000bebc44782c7db0a1a60cb6fe97d0b483032ff1c7"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000160"
      // callData
      "0000000000000000000000000000000000000000000000000000000000000084"
      // function selector
      "3df02124"
      // callData.tokenInIndex
      "0000000000000000000000000000000000000000000000000000000000000001"
      // callData.tokenOutIndex
      "0000000000000000000000000000000000000000000000000000000000000000"
      // callData.amountIn
      "0000000000000000000000000000000000000000000000000000000000005ba9"
      // callData.amountOutMinimum
      "0000000000000000000000000000000000000000000000000053259bfcdc6572"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"
      "0000000000000000000000000000000000000000000000000000000000000002"

      /*************************** calls[5] ***************************/
      // callType
      "0000000000000000000000000000000000000000000000000000000000000001"
      // target
      "0000000000000000000000006b175474e89094c44da98b954eedeac495271d0f"
      // value
      "0000000000000000000000000000000000000000000000000000000000000000"
      // offset to start data part of callData
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // offset to start data part of payload
      "0000000000000000000000000000000000000000000000000000000000000120"
      // callData
      // size(callData) = 68
      "0000000000000000000000000000000000000000000000000000000000000044"
      // function selector
      "a9059cbb"
      "000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4"
      "0000000000000000000000000000000000000000000000000000000000000000"
      // padding of (32 - 4) bytes for function selector
      "00000000000000000000000000000000000000000000000000000000"

      // payload
      // size(payload) = 64
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000006b175474e89094c44da98b954eedeac495271d0f"
      "0000000000000000000000000000000000000000000000000000000000000001"

      // extraneous calldata (ignored)
      "9d9cfc2dfc8fc0f9b038d3a284250391",
      &data));

  auto tx_info =
      GetTransactionInfoFromData(mojom::kMainnetChainId, kSquidRouter, data);
  ASSERT_NE(tx_info, std::nullopt);

  std::tie(tx_type, tx_params, tx_args, swap_info) = std::move(*tx_info);

  EXPECT_EQ(tx_type, mojom::TransactionType::ETHSwap);
  ASSERT_TRUE(swap_info);

  EXPECT_EQ(swap_info->source_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->source_chain_id, "");
  EXPECT_EQ(swap_info->source_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");  // ETH
  EXPECT_EQ(swap_info->source_amount, "0x9184e72a000");     // 0.00001 ETH

  EXPECT_EQ(swap_info->destination_coin, mojom::CoinType::ETH);
  EXPECT_EQ(swap_info->destination_chain_id, "");
  EXPECT_EQ(swap_info->destination_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");  // DAI
  EXPECT_EQ(swap_info->destination_amount, "");
  EXPECT_EQ(swap_info->destination_amount_min,
            "0x53259bfcdc6572");  // 0.02340377495944536 DAI

  EXPECT_EQ(swap_info->recipient, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(swap_info->provider, mojom::SwapProvider::kSquid);
}

}  // namespace brave_wallet
