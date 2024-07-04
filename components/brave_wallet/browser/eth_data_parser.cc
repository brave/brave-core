/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <map>
#include <optional>
#include <tuple>
#include <utility>

#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {

constexpr char kNativeEVMAssetContractAddress[] =
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
constexpr char kLiFiSwapTokensGeneric[] = "0x4630a0d8";
constexpr char kLiFiSwapTokensSingleErc20ToErc20[] = "0x878863a4";
constexpr char kLiFiSwapTokensSingleErc20ToNative[] = "0xd5bc7be1";
constexpr char kLiFiSwapTokensSingleNativeToErc20[] = "0x8f0af374";

struct LiFiSwapData {
  std::string from_amount;
  std::string sending_asset_id;
  std::string receiving_asset_id;
};

std::string TransformLiFiAddress(const std::string& address) {
  if (address == kLiFiNativeEVMAssetContractAddress) {
    return kNativeEVMAssetContractAddress;
  }

  return address;
}

// ABI for SwapData from LibSwap.sol in LiFi contracts:
//   (address callTo,
//    address approveTo,
//    address sendingAssetId,
//    address receivingAssetId,
//    uint256 fromAmount,
//    bytes callData,
//    bool requiresDeposit)
//
// Ref:
// https://github.com/lifinance/contracts/blob/be045f90cec0fdd272417a2da7fc960c5be046c2/src/Libraries/LibSwap.sol#L10-L18
eth_abi::Type MakeLiFiSwapDataType() {
  return eth_abi::Tuple()
      .AddTupleType(eth_abi::Address())
      .AddTupleType(eth_abi::Address())
      .AddTupleType(eth_abi::Address())
      .AddTupleType(eth_abi::Address())
      .AddTupleType(eth_abi::Uint(256))
      .AddTupleType(eth_abi::Bytes())
      .AddTupleType(eth_abi::Bool())
      .build();
}

std::optional<LiFiSwapData> LiFiSwapDataDecode(
    const base::Value::List& swap_data_list) {
  if (swap_data_list.size() == 1) {
    // Direct swap.
    auto& swap_data = swap_data_list.front().GetList();
    return LiFiSwapData{
        .from_amount = swap_data[4].GetString(),
        .sending_asset_id = TransformLiFiAddress(swap_data[2].GetString()),
        .receiving_asset_id = TransformLiFiAddress(swap_data[3].GetString()),
    };
  } else if (swap_data_list.size() >= 2) {
    // Multi-hop swap.
    //
    // We are only interested in the first and last elements of the
    // array, which represent the sending and receiving assets, respectively.
    auto& swap_data_front = swap_data_list.front().GetList();
    auto& swap_data_back = swap_data_list.back().GetList();

    return LiFiSwapData{
        .from_amount = swap_data_front[4].GetString(),
        .sending_asset_id =
            TransformLiFiAddress(swap_data_front[2].GetString()),
        .receiving_asset_id =
            TransformLiFiAddress(swap_data_back[3].GetString()),
    };
  }

  // Invalid transaction.
  return std::nullopt;
}

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
    auto type = eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ETHFilForwarderTransfer,
        std::vector<std::string>{"bytes"},  // recipient
        std::vector<std::string>{decoded.value()[0].GetString()});

  } else if (selector == kERC20TransferSelector) {
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ERC20Transfer,
        std::vector<std::string>{"address",   // recipient
                                 "uint256"},  // amount
        std::vector<std::string>{decoded.value()[0].GetString(),
                                 decoded.value()[1].GetString()});
  } else if (selector == kERC20ApproveSelector) {
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ERC20Approve,
        std::vector<std::string>{"address",   // spender
                                 "uint256"},  // amount
        std::vector<std::string>{decoded.value()[0].GetString(),
                                 decoded.value()[1].GetString()});
  } else if (selector == kERC721TransferFromSelector) {
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ERC721TransferFrom,
        std::vector<std::string>{"address",   // to
                                 "address",   // from
                                 "uint256"},  // tokenId
        std::vector<std::string>{decoded.value()[0].GetString(),
                                 decoded.value()[1].GetString(),
                                 decoded.value()[2].GetString()});
  } else if (selector == kERC721SafeTransferFromSelector) {
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ERC721SafeTransferFrom,
        std::vector<std::string>{"address",   // to
                                 "address",   // from
                                 "uint256"},  // tokenId
        std::vector<std::string>{decoded.value()[0].GetString(),
                                 decoded.value()[1].GetString(),
                                 decoded.value()[2].GetString()});
  } else if (selector == kSellEthForTokenToUniswapV3Selector) {
    // Function:
    // sellEthForTokenToUniswapV3(bytes encodedPath,
    //                            uint256 minBuyAmount,
    //                            address recipient)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/b46eeadc64485288add5940a210e1a7d0bcb5481/contracts/zero-ex/contracts/src/features/interfaces/IUniswapV3Feature.sol#L29-L41
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Bytes())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Address())
                    .build();
    auto decoded_calldata = ABIDecode(type, calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    auto decoded_path =
        UniswapEncodedPathDecode(decoded_calldata.value()[0].GetString());
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
                                 decoded_calldata.value()[1].GetString()});
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
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Bytes())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Address())
                    .build();
    auto decoded_calldata = ABIDecode(type, calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    auto decoded_path =
        UniswapEncodedPathDecode(decoded_calldata.value()[0].GetString());
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
                                 decoded_calldata.value()[1].GetString(),
                                 decoded_calldata.value()[2].GetString()});
  } else if (selector == kSellToUniswapSelector) {
    // Function:
    // sellToUniswap(address[] tokens,
    //               uint256 sellAmount,
    //               uint256 minBuyAmount,
    //               bool isSushi)
    //
    // Ref:
    // https://github.com/0xProject/protocol/blob/8d6f6e76e053f7b065d3315ddb31d2c35caddca7/contracts/zero-ex/contracts/src/features/UniswapFeature.sol#L93-L104
    auto type =
        eth_abi::Tuple()
            .AddTupleType(
                eth_abi::Array().SetArrayType(eth_abi::Address()).build())
            .AddTupleType(eth_abi::Uint(256))
            .AddTupleType(eth_abi::Uint(256))
            .AddTupleType(eth_abi::Bool())
            .build();
    auto decoded_calldata = ABIDecode(type, calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    std::string fill_path = "0x";
    for (const auto& address : decoded_calldata.value()[0].GetList()) {
      if (!address.is_string()) {
        return std::nullopt;
      }

      base::StrAppend(&fill_path, {address.GetString().substr(2)});
    }

    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // maker amount
                                 "uint256"},  // taker amount
        std::vector<std::string>{fill_path,
                                 decoded_calldata.value()[1].GetString(),
                                 decoded_calldata.value()[2].GetString()});
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
    auto type =
        eth_abi::Tuple()
            .AddTupleType(eth_abi::Address())
            .AddTupleType(eth_abi::Address())
            .AddTupleType(eth_abi::Uint(256))
            .AddTupleType(eth_abi::Uint(256))
            .AddTupleType(eth_abi::Array()
                              .SetArrayType(eth_abi::Tuple()
                                                .AddTupleType(eth_abi::Uint(32))
                                                .AddTupleType(eth_abi::Bytes())
                                                .build())
                              .build())
            .build();
    auto decoded_calldata = ABIDecode(type, calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // maker amount
                                 "uint256"},  // taker amount
        std::vector<std::string>{
            decoded_calldata.value()[0].GetString() +
                decoded_calldata.value()[1].GetString().substr(2),
            decoded_calldata.value()[2].GetString(),
            decoded_calldata.value()[3].GetString()});
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
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(128))
                    .AddTupleType(eth_abi::Uint(128))
                    .build();
    auto decoded_calldata = ABIDecode(type, calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    std::vector<std::string> tx_args;

    if (selector == kFillOtcOrderForEthSelector) {
      // The output of the swap is actually WETH but fillOtcOrderForEth()
      // automatically unwraps it to ETH. The buyToken is therefore the
      // 0x native asset contract.
      tx_args = {decoded_calldata.value()[1].GetString() +
                     std::string(kNativeEVMAssetContractAddress).substr(2),
                 decoded_calldata.value()[3].GetString(),
                 decoded_calldata.value()[2].GetString()};
    } else if (selector == kFillOtcOrderWithEthSelector) {
      // The input of the swap is actually ETH but fillOtcOrderWithEth()
      // automatically wraps it to WETH. The sellToken is therefore the 0x
      // native asset contract.
      //
      // Clients are free to use the sellAmount extracted from calldata or
      // the value field of the swap transaction. The latter is more reliable
      // since OTC trades may include protocol fees payable in ETH that get
      // added to the sellAmount.
      tx_args = {std::string(kNativeEVMAssetContractAddress) +
                     decoded_calldata.value()[0].GetString().substr(2),
                 decoded_calldata.value()[3].GetString(),
                 decoded_calldata.value()[2].GetString()};
    } else if (selector == kFillOtcOrderSelector) {
      tx_args = {decoded_calldata.value()[1].GetString() +
                     decoded_calldata.value()[0].GetString().substr(2),
                 decoded_calldata.value()[3].GetString(),
                 decoded_calldata.value()[2].GetString()};
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
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .build();
    auto decoded_calldata = ABIDecode(type, calldata);
    if (!decoded_calldata) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // sell amount
                                 "uint256"},  // buy amount
        std::vector<std::string>{
            std::string(kNativeEVMAssetContractAddress) +
                decoded_calldata.value()[0].GetString().substr(2),
            decoded_calldata.value()[2].GetString(),
            decoded_calldata.value()[3].GetString()});
  } else if (selector == kLiFiSwapTokensGeneric) {
    // The following block handles decoding of calldata for generic LiFi swap
    // orders using GenericSwapFacet.
    //
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
    //
    // Ref:
    // https://github.com/lifinance/contracts/blob/fe89ad34a9d2bff4dbf27c2d09b71363c282cd0b/src/Facets/GenericSwapFacet.sol#L207-L221
    auto type =
        eth_abi::Tuple()
            .AddTupleType(eth_abi::Bytes(32))
            .AddTupleType(eth_abi::String())
            .AddTupleType(eth_abi::String())
            .AddTupleType(eth_abi::Address())
            .AddTupleType(eth_abi::Uint(256))
            .AddTupleType(
                eth_abi::Array().SetArrayType(MakeLiFiSwapDataType()).build())
            .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    CHECK_EQ(6u, decoded.value().size());

    auto min_amount_out = decoded.value()[4].GetString();

    // The swapData field is an array of tuples, each representing a swap fill
    // operation.
    auto swap_data = LiFiSwapDataDecode(decoded.value()[5].GetList());
    if (!swap_data) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // sell amount
                                 "uint256"},  // buy amount
        std::vector<std::string>{swap_data->sending_asset_id +
                                     swap_data->receiving_asset_id.substr(2),
                                 swap_data->from_amount, min_amount_out});
  } else if (selector == kLiFiSwapTokensSingleErc20ToErc20 ||
             selector == kLiFiSwapTokensSingleErc20ToNative ||
             selector == kLiFiSwapTokensSingleNativeToErc20) {
    // The following block handles decoding of calldata for generic LiFi swap
    // orders using GenericSwapFacetV3. The following cases are handled:
    //
    // TXN: token → token
    // Function:
    // swapTokensSingleV3ERC20ToERC20(bytes32 transactionId,
    //                                string integrator,
    //                                string referrer,
    //                                address receiver,
    //                                uint256 minAmountOut,
    //                                (address callTo,
    //                                 address approveTo,
    //                                 address sendingAssetId,
    //                                 address receivingAssetId,
    //                                 uint256 fromAmount,
    //                                 bytes callData,
    //                                 bool requiresDeposit) swapData)
    //
    // Ref:
    // https://github.com/lifinance/contracts/blob/0becf25cb5983e88d58636b6215b5a7aa1b267e0/src/Facets/GenericSwapFacetV3.sol#L25-L39
    //
    // TXN: token → ETH
    // Function:
    // swapTokensSingleV3ERC20ToNative(bytes32 transactionId,
    //                                 string integrator,
    //                                 string referrer,
    //                                 address receiver,
    //                                 uint256 minAmountOut,
    //                                 (address callTo,
    //                                  address approveTo,
    //                                  address sendingAssetId,
    //                                  address receivingAssetId,
    //                                  uint256 fromAmount,
    //                                  bytes callData,
    //                                  bool requiresDeposit) swapData)
    //
    // Ref:
    // https://github.com/lifinance/contracts/blob/0becf25cb5983e88d58636b6215b5a7aa1b267e0/src/Facets/GenericSwapFacetV3.sol#L81-L95
    //
    // TXN: ETH → token
    // Function:
    // swapTokensSingleV3NativeToERC20(bytes32 transactionId,
    //                                 string integrator,
    //                                 string referrer,
    //                                 address receiver,
    //                                 uint256 minAmountOut,
    //                                 (address callTo,
    //                                  address approveTo,
    //                                  address sendingAssetId,
    //                                  address receivingAssetId,
    //                                  uint256 fromAmount,
    //                                  bytes callData,
    //                                  bool requiresDeposit) swapData)
    //
    // Ref:
    // https://github.com/lifinance/contracts/blob/0becf25cb5983e88d58636b6215b5a7aa1b267e0/src/Facets/GenericSwapFacetV3.sol#L135-L149

    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Bytes(32))
                    .AddTupleType(eth_abi::String())
                    .AddTupleType(eth_abi::String())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(MakeLiFiSwapDataType())
                    .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    CHECK_EQ(6u, decoded.value().size());

    auto min_amount_out = decoded.value()[4].GetString();

    auto swap_data_list = base::Value::List();
    swap_data_list.Append(std::move(decoded.value()[5]));
    auto swap_data = LiFiSwapDataDecode(swap_data_list);
    if (!swap_data) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ETHSwap,
        std::vector<std::string>{"bytes",     // fill path,
                                 "uint256",   // sell amount
                                 "uint256"},  // buy amount
        std::vector<std::string>{swap_data->sending_asset_id +
                                     swap_data->receiving_asset_id.substr(2),
                                 swap_data->from_amount, min_amount_out});
  } else if (selector == kERC1155SafeTransferFromSelector) {
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Bytes())
                    .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ERC1155SafeTransferFrom,
        std::vector<std::string>{"address",  // from
                                 "address",  // to
                                 "uint256",  // id
                                 "uint256",  // amount
                                 "bytes"},   // data
        std::vector<std::string>{
            decoded.value()[0].GetString(), decoded.value()[1].GetString(),
            decoded.value()[2].GetString(), decoded.value()[3].GetString(),
            decoded.value()[4].GetString()});
  } else {
    return std::make_tuple(mojom::TransactionType::Other,
                           std::vector<std::string>(),
                           std::vector<std::string>());
  }
}

}  // namespace brave_wallet
