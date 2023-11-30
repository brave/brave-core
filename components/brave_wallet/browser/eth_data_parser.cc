/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <map>
#include <optional>
#include <tuple>

#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {

constexpr char kNativeAssetContractAddress[] =
    "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";

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
constexpr char kFillOtcOrderForEthSelector[] = "0xa578efaf";
constexpr char kFillOtcOrderWithEthSelector[] = "0x706394d5";
constexpr char kFillOtcOrderSelector[] = "0xdac748d4";
constexpr char kFilForwarderTransferSelector[] =
    "0xd948d468";  // forward(bytes)
constexpr char kCowOrderSellEthSelector[] = "0x322bba21";

}  // namespace

std::optional<std::tuple<mojom::TransactionType,     // tx_type
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
  if (selector == kFilForwarderTransferSelector) {
    auto decoded = ABIDecode({"bytes"}, calldata);
    if (!decoded) {
      return std::nullopt;
    }
    const auto& tx_args = std::get<1>(*decoded);
    if (tx_args.empty()) {
      return std::nullopt;
    }
    return std::make_tuple(mojom::TransactionType::ETHFilForwarderTransfer,
                           std::vector<std::string>{"bytes"},  // recipient
                           std::vector<std::string>{tx_args.at(0)});

  } else if (selector == kERC20TransferSelector) {
    auto decoded = ABIDecode({"address", "uint256"}, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::tuple_cat(
        std::make_tuple(mojom::TransactionType::ERC20Transfer), *decoded);
  } else if (selector == kERC20ApproveSelector) {
    auto decoded = ABIDecode({"address", "uint256"}, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::tuple_cat(std::make_tuple(mojom::TransactionType::ERC20Approve),
                          *decoded);
  } else if (selector == kERC721TransferFromSelector) {
    auto decoded = ABIDecode({"address", "address", "uint256"}, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::tuple_cat(
        std::make_tuple(mojom::TransactionType::ERC721TransferFrom), *decoded);
  } else if (selector == kERC721SafeTransferFromSelector) {
    auto decoded = ABIDecode({"address", "address", "uint256"}, calldata);
    if (!decoded) {
      return std::nullopt;
    }

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
    if (!decoded_calldata) {
      return std::nullopt;
    }

    const auto& tx_args = std::get<1>(*decoded_calldata);
    auto decoded_path = UniswapEncodedPathDecode(tx_args.at(0));
    if (!decoded_path) {
      return std::nullopt;
    }

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
    if (!decoded_calldata) {
      return std::nullopt;
    }

    const auto& tx_args = std::get<1>(*decoded_calldata);
    auto decoded_path = UniswapEncodedPathDecode(tx_args.at(0));
    if (!decoded_path) {
      return std::nullopt;
    }
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
    if (!decoded_calldata) {
      return std::nullopt;
    }

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
    if (!decoded_calldata) {
      return std::nullopt;
    }

    const auto& tx_args = std::get<1>(*decoded_calldata);
    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // maker amount
                                 "uint256"},  // taker amount
        std::vector<std::string>{tx_args.at(0) + tx_args.at(1).substr(2),
                                 tx_args.at(2), tx_args.at(3)});
  } else if (selector == kFillOtcOrderForEthSelector ||
             selector == kFillOtcOrderWithEthSelector ||
             selector == kFillOtcOrderSelector) {
    // The following block handles decoding of calldata for 0x OTC orders.
    // These orders are filled by professional market makers using the RFQ-T
    // system by 0x.

    // TXN: token → ETH
    // Function:
    // fillOtcOrderForEth((address buyToken,
    //                     address sellToken,
    //                     uint128 buyAmount,
    //                     uint128 sellAmount,
    //                     address maker,
    //                     address taker,
    //                     address txOrigin,
    //                     uint256 expiryAndNonce),
    //                    (uint8 signatureType,
    //                     uint8 v,
    //                     bytes32 r,
    //                     bytes32 s),
    //                    uint128 takerTokenFillAmount)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/bcbfbfa16c2ec98e64cd1f2f2f55a134baf3dbf6/contracts/zero-ex/contracts/src/features/OtcOrdersFeature.sol#L109-L113

    // TXN: ETH → token
    // Function:
    // fillOtcOrderWithEth((address buyToken,
    //                      address sellToken,
    //                      uint128 buyAmount,
    //                      uint128 sellAmount,
    //                      address maker,
    //                      address taker,
    //                      address txOrigin,
    //                      uint256 expiryAndNonce),
    //                     (uint8 signatureType,
    //                      uint8 v,
    //                      bytes32 r,
    //                      bytes32 s))
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/bcbfbfa16c2ec98e64cd1f2f2f55a134baf3dbf6/contracts/zero-ex/contracts/src/features/OtcOrdersFeature.sol#L139-L148

    // TXN: token → token
    // Function:
    // fillOtcOrder((address buyToken,
    //               address sellToken,
    //               uint128 buyAmount,
    //               uint128 sellAmount,
    //               address maker,
    //               address taker,
    //               address txOrigin,
    //               uint256 expiryAndNonce),
    //              (uint8 signatureType,
    //               uint8 v,
    //               bytes32 r,
    //               bytes32 s),
    //              uint128 takerTokenFillAmount)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/bcbfbfa16c2ec98e64cd1f2f2f55a134baf3dbf6/contracts/zero-ex/contracts/src/features/OtcOrdersFeature.sol#L68C6-L79

    // NOTE: tuples with static types can be flattened for easier decoding.
    // For example, fillOtcOrder() takes three arguments, the first two being
    // tuples. However, we can also consider this function to be taking 13
    // arguments.
    //
    // For the purpose of parsing transaction data corresponding to ETHSwap, we
    // are only interested in the first four fields. Ignore the rest of the
    // arguments as extraneous data.
    auto decoded_calldata = ABIDecode(
        {
            "address",  // buyToken
            "address",  // sellToken
            "uint128",  // buyAmount
            "uint128",  // sellAmount
        },
        calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    const auto& raw_args = std::get<1>(*decoded_calldata);
    std::vector<std::string> tx_args;

    if (selector == kFillOtcOrderForEthSelector) {
      // The output of the swap is actually WETH but fillOtcOrderForEth()
      // automatically unwraps it to ETH. The buyToken is therefore the
      // 0x native asset contract.
      tx_args = {
          raw_args.at(1) + std::string(kNativeAssetContractAddress).substr(2),
          raw_args.at(3), raw_args.at(2)};
    } else if (selector == kFillOtcOrderWithEthSelector) {
      // The input of the swap is actually ETH but fillOtcOrderWithEth()
      // automatically wraps it to WETH. The sellToken is therefore the 0x
      // native asset contract.
      //
      // Clients are free to use the sellAmount extracted from calldata or
      // the value field of the swap transaction. The latter is more reliable
      // since OTC trades may include protocol fees payable in ETH that get
      // added to the sellAmount.
      tx_args = {
          std::string(kNativeAssetContractAddress) + raw_args.at(0).substr(2),
          raw_args.at(3), raw_args.at(2)};
    } else if (selector == kFillOtcOrderSelector) {
      tx_args = {raw_args.at(1) + raw_args.at(0).substr(2), raw_args.at(3),
                 raw_args.at(2)};
    }

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{"bytes",     // fill path,
                                                    "uint128",   // sell amount
                                                    "uint128"},  // buy amount
                           tx_args);
  } else if (selector == kCowOrderSellEthSelector) {
    // The following block handles decoding of calldata for CoW swap orders,
    // when the sell asset is the native asset (ETH, XDAI, etc).

    // TXN: ETH/XDAI → token
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
    //
    // Refs:
    //   https://github.com/cowprotocol/ethflowcontract/blob/1d5d54a4ba890c5c0d3b26429ee32aa8e69f2f0d/src/CoWSwapEthFlow.sol#L81
    //   https://github.com/cowprotocol/ethflowcontract/blob/1d5d54a4ba890c5c0d3b26429ee32aa8e69f2f0d/src/libraries/EthFlowOrder.sol#L18-L45

    // NOTE: createOrder() takes one argument of type EthFlowOrder.Data, which
    // could be represented as a tuple. Since tuples with static types can be
    // flattened for easier decoding, we can consider this function to be
    // taking 9 arguments.
    //
    // For the purpose of parsing transaction data corresponding to ETHSwap, we
    // are only interested in the first four fields. Ignore the rest of the
    // arguments as extraneous data.
    auto decoded_calldata = ABIDecode(
        {
            "address",  // buyToken
            "address",  // receiver
            "uint256",  // sellAmount
            "uint256",  // buyAmount
        },
        calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    const auto& tx_args = std::get<1>(*decoded_calldata);
    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // sell amount
                                 "uint256"},  // buy amount
        std::vector<std::string>{
            std::string(kNativeAssetContractAddress) + tx_args.at(0).substr(2),
            tx_args.at(2), tx_args.at(3)});
  } else if (selector == kERC1155SafeTransferFromSelector) {
    auto decoded = ABIDecode(
        {"address", "address", "uint256", "uint256", "bytes"}, calldata);
    if (!decoded) {
      return std::nullopt;
    }

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
