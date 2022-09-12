/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <map>
#include <tuple>

#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {

constexpr char kERC20TransferSelector[] = "0xa9059cbb";
constexpr char kERC20ApproveSelector[] = "0x095ea7b3";
constexpr char kERC721TransferFromSelector[] = "0x23b872dd";
constexpr char kERC721SafeTransferFromSelector[] = "0x42842e0e";
constexpr char kERC1155SafeTransferFromSelector[] = "0xf242432a";
constexpr char kSellEthForTokenToUniswapV3Selector[] = "0x3598d8ab";
constexpr char kSellTokenForEthToUniswapV3Selector[] = "0x803ba26d";
constexpr char kSellTokenForTokenToUniswapV3Selector[] = "0x6af479b2";
constexpr char kSellToUniswapSelector[] = "0xd9627aa4";
constexpr char kTransformERC20Selector[] = "0x415565b0";

}  // namespace

absl::optional<std::tuple<mojom::TransactionType,     // tx_type
                          std::vector<std::string>,   // tx_params
                          std::vector<std::string>>>  // tx_args
GetTransactionInfoFromData(const std::vector<uint8_t>& data) {
  if (data.empty() || data == std::vector<uint8_t>{0x0}) {
    return std::make_tuple(mojom::TransactionType::ETHSend,
                           std::vector<std::string>(),
                           std::vector<std::string>());
  }

  if (data.size() < 4) {
    return std::make_tuple(mojom::TransactionType::Other,
                           std::vector<std::string>(),
                           std::vector<std::string>());
  }

  std::string selector = "0x" + HexEncodeLower(data.data(), 4);
  std::vector<uint8_t> calldata(data.begin() + 4, data.end());
  if (selector == kERC20TransferSelector) {
    auto decoded = ABIDecode({"address", "uint256"}, calldata);
    if (!decoded)
      return absl::nullopt;

    return std::tuple_cat(
        std::make_tuple(mojom::TransactionType::ERC20Transfer), *decoded);
  } else if (selector == kERC20ApproveSelector) {
    auto decoded = ABIDecode({"address", "uint256"}, calldata);
    if (!decoded)
      return absl::nullopt;

    return std::tuple_cat(std::make_tuple(mojom::TransactionType::ERC20Approve),
                          *decoded);
  } else if (selector == kERC721TransferFromSelector) {
    auto decoded = ABIDecode({"address", "address", "uint256"}, calldata);
    if (!decoded)
      return absl::nullopt;

    return std::tuple_cat(
        std::make_tuple(mojom::TransactionType::ERC721TransferFrom), *decoded);
  } else if (selector == kERC721SafeTransferFromSelector) {
    auto decoded = ABIDecode({"address", "address", "uint256"}, calldata);
    if (!decoded)
      return absl::nullopt;

    return std::tuple_cat(
        std::make_tuple(mojom::TransactionType::ERC721SafeTransferFrom),
        *decoded);
  } else if (selector == kSellEthForTokenToUniswapV3Selector) {
    // Function:
    // sellEthForTokenToUniswapV3(bytes encodedPath,
    //                            uint256 minBuyAmount,
    //                            address recipient)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/b46eeadc64485288add5940a210e1a7d0bcb5481/contracts/zero-ex/contracts/src/features/interfaces/IUniswapV3Feature.sol#L29-L41
    auto decoded_calldata =
        ABIDecode({"bytes", "uint256", "address"}, calldata);
    if (!decoded_calldata)
      return absl::nullopt;

    const auto& tx_args = std::get<1>(*decoded_calldata);
    auto decoded_path = UniswapEncodedPathDecode(tx_args.at(0));
    if (!decoded_path)
      return absl::nullopt;

    std::string fill_path = "0x";
    for (const auto& path : *decoded_path) {
      base::StrAppend(&fill_path, {path.substr(2)});
    }

    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // maker amount
                                 "uint256"},  // taker amount
        std::vector<std::string>{fill_path,
                                 "",  // maker asset is ETH, amount is txn value
                                 tx_args.at(1)});
  } else if (selector == kSellTokenForEthToUniswapV3Selector ||
             selector == kSellTokenForTokenToUniswapV3Selector) {
    // Function: 0x803ba26d
    // sellTokenForEthToUniswapV3(bytes encodedPath,
    //                            uint256 sellAmount,
    //                            uint256 minBuyAmount,
    //                            address recipient)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/b46eeadc64485288add5940a210e1a7d0bcb5481/contracts/zero-ex/contracts/src/features/interfaces/IUniswapV3Feature.sol#L43-L56
    //
    //
    // Function: 0x6af479b2
    // sellTokenForTokenToUniswapV3(bytes encodedPath,
    //                              uint256 sellAmount,
    //                              uint256 minBuyAmount,
    //                              address recipient)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/b46eeadc64485288add5940a210e1a7d0bcb5481/contracts/zero-ex/contracts/src/features/interfaces/IUniswapV3Feature.sol#L58-L71
    auto decoded_calldata =
        ABIDecode({"bytes", "uint256", "uint256", "address"}, calldata);
    if (!decoded_calldata)
      return absl::nullopt;

    const auto& tx_args = std::get<1>(*decoded_calldata);
    auto decoded_path = UniswapEncodedPathDecode(tx_args.at(0));
    if (!decoded_path)
      return absl::nullopt;
    std::string fill_path = "0x";
    for (const auto& path : *decoded_path) {
      base::StrAppend(&fill_path, {path.substr(2)});
    }

    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // maker amount
                                 "uint256"},  // taker amount
        std::vector<std::string>{fill_path, tx_args.at(1), tx_args.at(2)});
  } else if (selector == kSellToUniswapSelector) {
    // Function:
    // sellToUniswap(address[] tokens,
    //               uint256 sellAmount,
    //               uint256 minBuyAmount,
    //               bool isSushi)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/8d6f6e76e053f7b065d3315ddb31d2c35caddca7/contracts/zero-ex/contracts/src/features/UniswapFeature.sol#L93-L104
    auto decoded_calldata =
        ABIDecode({"address[]", "uint256", "uint256", "bool"}, calldata);
    if (!decoded_calldata)
      return absl::nullopt;

    const auto& tx_args = std::get<1>(*decoded_calldata);
    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // maker amount
                                 "uint256"},  // taker amount
        std::vector<std::string>{tx_args.at(0), tx_args.at(1), tx_args.at(2)});
  } else if (selector == kTransformERC20Selector) {
    // Function:
    // transformERC20(address inputToken,
    //                address outputToken,
    //                uint256 inputTokenAmount,
    //                uint256 minOutputTokenAmount,
    //                (uint32,bytes)[] transformations)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/b46eeadc64485288add5940a210e1a7d0bcb5481/contracts/zero-ex/contracts/src/features/interfaces/ITransformERC20Feature.sol#L113-L134
    auto decoded_calldata = ABIDecode(
        {"address", "address", "uint256", "uint256", "(uint32,bytes)[]"},
        calldata);
    if (!decoded_calldata)
      return absl::nullopt;

    const auto& tx_args = std::get<1>(*decoded_calldata);
    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // maker amount
                                 "uint256"},  // taker amount
        std::vector<std::string>{tx_args.at(0) + tx_args.at(1).substr(2),
                                 tx_args.at(2), tx_args.at(3)});
  } else if (selector == kERC1155SafeTransferFromSelector) {
    auto decoded = ABIDecode(
        {"address", "address", "uint256", "uint256", "bytes"}, calldata);
    if (!decoded)
      return absl::nullopt;

    return std::tuple_cat(
        std::make_tuple(mojom::TransactionType::ERC1155SafeTransferFrom),
        *decoded);
  } else {
    return std::make_tuple(mojom::TransactionType::Other,
                           std::vector<std::string>(),
                           std::vector<std::string>());
  }
}

}  // namespace brave_wallet
