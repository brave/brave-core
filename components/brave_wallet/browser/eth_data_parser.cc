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

constexpr char kERC20TransferSelector[] = "0xa9059cbb";
constexpr char kERC20ApproveSelector[] = "0x095ea7b3";
constexpr char kERC721TransferFromSelector[] = "0x23b872dd";
constexpr char kERC721SafeTransferFromSelector[] = "0x42842e0e";
constexpr char kERC1155SafeTransferFromSelector[] = "0xf242432a";
constexpr char kFilForwarderTransferSelector[] =
    "0xd948d468";  // forward(bytes)

// CowSwap function selectors
constexpr char kCowOrderSellEthSelector[] = "0x322bba21";

// 0x function selectors
constexpr char kSellEthForTokenToUniswapV3Selector[] = "0x3598d8ab";
constexpr char kSellTokenForEthToUniswapV3Selector[] = "0x803ba26d";
constexpr char kSellTokenForTokenToUniswapV3Selector[] = "0x6af479b2";
constexpr char kSellToUniswapSelector[] = "0xd9627aa4";
constexpr char kTransformERC20Selector[] = "0x415565b0";
constexpr char kFillOtcOrderForEthSelector[] = "0xa578efaf";
constexpr char kFillOtcOrderWithEthSelector[] = "0x706394d5";
constexpr char kFillOtcOrderSelector[] = "0xdac748d4";

// LiFi function selectors
// Ref: https://louper.dev/diamond/0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE

// AcrossFacet
constexpr char kLiFiStartBridgeTokensViaAcross[] = "0x1fd8010c";
constexpr char kLiFiSwapAndStartBridgeTokensViaAcross[] = "0x3a3f7332";

// AllBridgeFacet
constexpr char kLiFiStartBridgeTokensViaAllBridge[] = "0xe40f2460";
constexpr char kLiFiSwapAndStartBridgeTokensViaAllBridge[] = "0xa74ccb35";

// AmarokFacet
constexpr char kLiFiStartBridgeTokensViaAmarok[] = "0x8dc9932d";
constexpr char kLiFiSwapAndStartBridgeTokensViaAmarok[] = "0x83f31917";

// ArbitrumBridgeFacet
constexpr char kLiFiStartBridgeTokensViaArbitrumBridge[] = "0xc9851d0b";
constexpr char kLiFiSwapAndStartBridgeTokensViaArbitrumBridge[] = "0x3cc9517b";

// CBridgeFacet
constexpr char kLiFiStartBridgeTokensViaCBridge[] = "0xae0b91e5";
constexpr char kLiFiSwapAndStartBridgeTokensViaCBridge[] = "0x482c6a85";

// CelerCircleBridgeFacet
constexpr char kLiFiStartBridgeTokensViaCelerCircleBridge[] = "0xbab657d8";
constexpr char kLiFiSwapAndStartBridgeTokensViaCelerCircleBridge[] =
    "0x8fab0663";

// CelerIMFacetMutable
constexpr char kLiFiStartBridgeTokensViaCelerIM[] = "0x05095ded";
constexpr char kLiFiSwapAndStartBridgeTokensViaCelerIM[] = "0xb06c52da";

// GenericSwapFacet
constexpr char kLiFiSwapTokensGeneric[] = "0x4630a0d8";
constexpr char kLiFiSwapTokensSingleErc20ToErc20[] = "0x878863a4";
constexpr char kLiFiSwapTokensSingleErc20ToNative[] = "0xd5bc7be1";
constexpr char kLiFiSwapTokensSingleNativeToErc20[] = "0x8f0af374";

// GenericSwapFacetV3
constexpr char kLiFiSwapTokensMultipleV3ERC20ToERC20[] = "0x5fd9ae2e";
constexpr char kLiFiswapTokensMultipleV3ERC20ToNative[] = "0x2c57e884";
constexpr char kLiFiSwapTokensMultipleV3NativeToERC20[] = "0x736eac0b";
constexpr char kLiFiSwapTokensSingleV3ERC20ToERC20[] = "0x4666fc80";
constexpr char kLiFiSwapTokensSingleV3ERC20ToNative[] = "0x733214a3";
constexpr char kLiFiSwapTokensSingleV3NativeToERC20[] = "0xaf7060fd";

// GnosisBridgeFacet
constexpr char kLiFiStartBridgeTokensViaXDaiBridge[] = "0x02cba4a3";
constexpr char kLiFiSwapAndStartBridgeTokensViaXDaiBridge[] = "0xa9d0550f";

// HopFacet
constexpr char kLiFiStartBridgeTokensViaHop[] = "0xb3b63587";
constexpr char kLiFiSwapAndStartBridgeTokensViaHop[] = "0xa01fe784";

// HyphenFacet
constexpr char kLiFiStartBridgeTokensViaHyphen[] = "0x8bf6ef99";
constexpr char kLiFiSwapAndStartBridgeTokensViaHyphen[] = "0x9feb6731";

// LIFuelFacet
constexpr char kLiFiStartBridgeTokensViaLIFuel[] = "0x9b6ee8e4";
constexpr char kLiFiSwapAndStartBridgeTokensViaLIFuel[] = "0x55206216";

// MayanFacet
constexpr char kLiFiStartBridgeTokensViaMayan[] = "0xb621b032";
constexpr char kLiFiSwapAndStartBridgeTokensViaMayan[] = "0x30c48952";

// MultichainFacet
constexpr char kLiFiStartBridgeTokensViaMultichain[] = "0xef55f6dd";
constexpr char kLiFiSwapAndStartBridgeTokensViaMultichain[] = "0xa342d3ff";

// OmniBridgeFacet
constexpr char kLiFiStartBridgeTokensViaOmniBridge[] = "0x782621d8";
constexpr char kLiFiSwapAndStartBridgeTokensViaOmniBridge[] = "0x95726782";

// OptimismBridgeFacet
constexpr char kLiFiStartBridgeTokensViaOptimismBridge[] = "0xce8a97a5";
constexpr char kLiFiSwapAndStartBridgeTokensViaOptimismBridge[] = "0x5bb5d448";

// PolygonBridgeFacet
constexpr char kLiFiStartBridgeTokensViaPolygonBridge[] = "0xaf62c7d6";
constexpr char kLiFiSwapAndStartBridgeTokensViaPolygonBridge[] = "0xb4f37581";

// SquidFacet
constexpr char kLiFiStartBridgeTokensViaSquid[] = "0x3f313808";
constexpr char kLiFiSwapAndStartBridgeTokensViaSquid[] = "0xa8f66666";

// StargateFacet
constexpr char kLiFiStartBridgeTokensViaStargate[] = "0xbe1eace7";
constexpr char kLiFiSwapAndStartBridgeTokensViaStargate[] = "0xed178619";

// StargateFacetV2
constexpr char kLiFiStartBridgeTokensViaStargateV2[] = "0x14d53077";
constexpr char kLiFiSwapAndStartBridgeTokensViaStargateV2[] = "0xa6010a66";

// SymbiosisFacet
constexpr char kLiFiStartBridgeTokensViaSymbiosis[] = "0xb70fb9a5";
constexpr char kLiFiSwapAndStartBridgeTokensViaSymbiosis[] = "0x6e067161";

// ThorSwapFacet
constexpr char kLiFiStartBridgeTokensViaThorSwap[] = "0x2541ec57";
constexpr char kLiFiSwapAndStartBridgeTokensViaThorSwap[] = "0xad673d88";

// Squid function selectors
// Ref:
// https://etherscan.io/address/0x9c01172bdbed2eea06e4e18ad534bf651c9089ea#code#F1#L87
constexpr char kSquidFundAndRunMulticall[] = "0x58181a80";
constexpr char kSquidExactInputSingle[] = "0x04e45aaf";
constexpr char kSquidExactInputSingleV2[] = "0xbc651188";
constexpr char kSquidSwapExactTokensForTokens[] = "0xcac88ea9";
constexpr char kSquidExchange[] = "0x3df02124";

struct LiFiSwapData {
  std::string from_amount;
  std::string sending_asset_id;
  std::string receiving_asset_id;
};

struct LiFiBridgeData {
  std::string bridge;
  std::string integrator;
  std::string sending_asset_id;
  std::string receiver;
  std::string min_amount;
  std::string destination_chain_id;
  mojom::CoinType destination_coin;
};

struct SquidSwapData {
  std::string to_amount;
  std::string receiving_asset_id;
  std::string receiver;
};

std::string TransformContractAddress(const std::string& address) {
  if (address == kLiFiNativeEVMAssetContractAddress || address.empty()) {
    return kNativeEVMAssetContractAddress;
  }

  return address;
}

std::string TransformEoaAddress(const std::string& address) {
  if (address == kLiFiNativeEVMAssetContractAddress) {
    return "";
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
        .sending_asset_id = TransformContractAddress(swap_data[2].GetString()),
        .receiving_asset_id =
            TransformContractAddress(swap_data[3].GetString()),
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
            TransformContractAddress(swap_data_front[2].GetString()),
        .receiving_asset_id =
            TransformContractAddress(swap_data_back[3].GetString()),
    };
  }

  // Invalid transaction.
  return std::nullopt;
}

std::optional<SquidSwapData> SquidDecodeCall(const base::Value::List& call) {
  auto calldata_with_selector = PrefixedHexStringToBytes(call[3].GetString());
  if (!calldata_with_selector) {
    return std::nullopt;
  }

  if (calldata_with_selector->size() == 0) {
    return SquidSwapData{
        .to_amount = "",  // Not available in native transfer.
        .receiving_asset_id = TransformContractAddress(""),
        .receiver = TransformEoaAddress(call[1].GetString()),
    };
  }

  // Calldata must have at least 4 bytes for the function selector.
  if (calldata_with_selector->size() < 4) {
    return std::nullopt;
  }

  auto [selector_span, calldata] =
      base::span(*calldata_with_selector).split_at<4>();

  auto selector = ToHex(selector_span);

  if (selector == kSquidExactInputSingle) {
    // exactInputSingle((address tokenIn,
    //                   address tokenOut,
    //                   uint24 fee,
    //                   address recipient,
    //                   uint256 amountIn,
    //                   uint256 amountOutMinimum,
    //                   uint160 sqrtPriceLimitX96))
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Bytes(32))
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Bytes(32))
                    .build();

    auto decoded_swap = ABIDecode(type, calldata);
    if (!decoded_swap) {
      return std::nullopt;
    }

    return SquidSwapData{
        .to_amount = decoded_swap.value()[5].GetString(),
        .receiving_asset_id =
            TransformContractAddress(decoded_swap.value()[1].GetString()),
        .receiver = TransformEoaAddress(decoded_swap.value()[3].GetString()),
    };
  } else if (selector == kSquidExactInputSingleV2) {
    // exactInputSingle((address tokenIn,
    //                   address tokenOut,
    //                   address recipient,
    //                   uint256 deadline,
    //                   uint256 amountIn,
    //                   uint256 amountOutMinimum,
    //                   uint160 sqrtPriceLimitX96))
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Bytes(32))
                    .build();

    auto decoded_swap = ABIDecode(type, calldata);
    if (!decoded_swap) {
      return std::nullopt;
    }

    return SquidSwapData{
        .to_amount = decoded_swap.value()[5].GetString(),
        .receiving_asset_id =
            TransformContractAddress(decoded_swap.value()[1].GetString()),
        .receiver = TransformEoaAddress(decoded_swap.value()[2].GetString()),
    };
  } else if (selector == kSquidSwapExactTokensForTokens) {
    // swapExactTokensForTokens(uint256 amountIn,
    //                          uint256 amountOutMin,
    //                          (address from,
    //                           address to,
    //                           bool stable,
    //                           address factory)[] routes,
    //                          address to,
    //                          uint256 deadline)
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(
                        eth_abi::Array()
                            .SetArrayType(eth_abi::Tuple()
                                              .AddTupleType(eth_abi::Address())
                                              .AddTupleType(eth_abi::Address())
                                              .AddTupleType(eth_abi::Bool())
                                              .AddTupleType(eth_abi::Address())
                                              .build())
                            .build())
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .build();

    auto decoded_swap = ABIDecode(type, calldata);
    if (!decoded_swap) {
      return std::nullopt;
    }

    auto& route = decoded_swap.value()[2].GetList().back().GetList();
    return SquidSwapData{
        .to_amount = decoded_swap.value()[1].GetString(),
        .receiving_asset_id = TransformContractAddress(route[1].GetString()),
        .receiver = TransformEoaAddress(decoded_swap.value()[3].GetString()),
    };
  } else if (selector == kERC20TransferSelector) {
    // transfer(address recipient, uint256 amount)
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .build();

    auto decoded_swap = ABIDecode(type, calldata);
    if (!decoded_swap) {
      return std::nullopt;
    }

    return SquidSwapData{
        .to_amount = "",
        .receiving_asset_id = call[1].GetString(),
        .receiver = TransformEoaAddress(decoded_swap.value()[0].GetString()),
    };
  } else if (selector == kSquidExchange) {
    // exchange(int128 tokenInIndex,
    //          int128 tokenOutIndex,
    //          uint256 amountIn,
    //          uint256 amountOutMin)
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Bytes(32))
                    .AddTupleType(eth_abi::Bytes(32))
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(eth_abi::Uint(256))
                    .build();

    auto decoded_swap = ABIDecode(type, calldata);
    if (!decoded_swap) {
      return std::nullopt;
    }

    return SquidSwapData{
        .to_amount = decoded_swap.value()[3].GetString(),
        .receiving_asset_id = "",
        .receiver = "",
    };
  }

  return std::nullopt;
}

std::optional<SquidSwapData> SquidDecodeMulticall(
    const base::Value::List& multicall) {
  std::optional<SquidSwapData> last_call_swap_data = std::nullopt;
  for (int i = multicall.size() - 1; i >= 0; --i) {
    auto& call = multicall[i].GetList();
    const auto& swap_data = SquidDecodeCall(call);
    if (!swap_data) {
      // Skip calls in the multicall series that are not swaps.
      if (last_call_swap_data.has_value()) {
        continue;
      }

      return std::nullopt;
    }

    // If the last call in the multicall chain does not contain certain fields,
    // we extract it from the current call.
    //
    // For example, if the last call in the multicall is a native transfer, we
    // extract the full swap data from the corresponding call for the wrapped
    // native token.
    if (last_call_swap_data.has_value()) {
      if (last_call_swap_data->to_amount.empty() &&
          !swap_data->to_amount.empty()) {
        last_call_swap_data->to_amount = swap_data->to_amount;
      }

      if (last_call_swap_data->receiving_asset_id.empty() &&
          !swap_data->receiving_asset_id.empty()) {
        last_call_swap_data->receiving_asset_id = swap_data->receiving_asset_id;
      }

      if (last_call_swap_data->receiver.empty() &&
          !swap_data->receiver.empty()) {
        last_call_swap_data->receiver = swap_data->receiver;
      }

      return last_call_swap_data;
    }

    if (swap_data->receiving_asset_id == kNativeEVMAssetContractAddress) {
      last_call_swap_data = swap_data;
      continue;
    }

    // If the last call in the multicall chain does not contain the to_amount,
    // we save the swap data for the next iteration.
    if (swap_data->to_amount.empty()) {
      last_call_swap_data = swap_data;
      continue;
    }

    return swap_data;
  }

  return std::nullopt;
}

// ABI for BridgeData from ILiFi.sol in LiFi contracts:
//
//   (bytes32 transactionId,
//    string bridge,
//    string integrator,
//    address referrer,
//    address sendingAssetId,
//    address receiver,
//    uint256 minAmount,
//    uint256 destinationChainId,
//    bool hasSourceSwaps,
//    bool hasDestinationCall)
//
// Ref:
// https://github.com/lifinance/contracts/blob/7063b60785428daab2a2decde67f52a53d74532f/src/Interfaces/ILiFi.sol#L7-L18
eth_abi::Type MakeLiFiBridgeDataType() {
  return eth_abi::Tuple()
      .AddTupleType(eth_abi::Bytes(32))
      .AddTupleType(eth_abi::String())
      .AddTupleType(eth_abi::String())
      .AddTupleType(eth_abi::Address())
      .AddTupleType(eth_abi::Address())
      .AddTupleType(eth_abi::Address())
      .AddTupleType(eth_abi::Uint(256))
      .AddTupleType(eth_abi::Uint(256))
      .AddTupleType(eth_abi::Bool())
      .AddTupleType(eth_abi::Bool())
      .build();
}

std::optional<LiFiBridgeData> LiFiBridgeDataDecode(
    const base::Value::List& data) {
  if (data.size() != 10) {
    return std::nullopt;
  }

  auto destination_chain_id = data[7].GetString();
  // Solana mainnet chain id used by LiFi encoded as hex string.
  // Ref: kLiFiSolanaMainnetChainID in browser/brave_wallet_constants.h
  if (destination_chain_id == "0x416edef1601be") {
    destination_chain_id = mojom::kSolanaMainnet;
  }

  return LiFiBridgeData{
      .bridge = data[1].GetString(),
      .integrator = data[2].GetString(),
      .sending_asset_id = TransformContractAddress(data[4].GetString()),
      .receiver = destination_chain_id == mojom::kSolanaMainnet
                      ? ""
                      : TransformEoaAddress(data[5].GetString()),
      .min_amount = data[6].GetString(),
      .destination_chain_id = destination_chain_id,
      .destination_coin = destination_chain_id == mojom::kSolanaMainnet
                              ? mojom::CoinType::SOL
                              : mojom::CoinType::ETH,
  };
}

}  // namespace

std::optional<std::tuple<mojom::TransactionType,    // tx_type
                         std::vector<std::string>,  // tx_params
                         std::vector<std::string>,  // tx_args
                         mojom::SwapInfoPtr>>       // swap_info
GetTransactionInfoFromData(const std::vector<uint8_t>& data) {
  if (data.empty() || data == std::vector<uint8_t>{0x0}) {
    return std::make_tuple(mojom::TransactionType::ETHSend,
                           std::vector<std::string>(),
                           std::vector<std::string>(), nullptr);
  }

  if (data.size() < 4) {
    return std::make_tuple(mojom::TransactionType::Other,
                           std::vector<std::string>(),
                           std::vector<std::string>(), nullptr);
  }

  auto [selector_span, calldata] = base::span(data).split_at<4>();

  std::string selector = ToHex(selector_span);
  if (selector == kFilForwarderTransferSelector) {
    auto type = eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    return std::make_tuple(
        mojom::TransactionType::ETHFilForwarderTransfer,
        std::vector<std::string>{"bytes"},  // recipient
        std::vector<std::string>{decoded.value()[0].GetString()}, nullptr);

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
                                 decoded.value()[1].GetString()},
        nullptr);
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
                                 decoded.value()[1].GetString()},
        nullptr);
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
                                 decoded.value()[2].GetString()},
        nullptr);
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
                                 decoded.value()[2].GetString()},
        nullptr);
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

    if (decoded_path->size() < 2) {
      return std::nullopt;
    }

    auto from_asset = decoded_path->front();
    auto to_asset = decoded_path->back();

    auto swap_info = mojom::SwapInfo::New();
    swap_info->from_coin = mojom::CoinType::ETH;
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_asset = kNativeEVMAssetContractAddress;
    swap_info->from_amount = "";  // asset is ETH, amount is txn value
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = to_asset;
    swap_info->to_amount = decoded_calldata.value()[1].GetString();
    swap_info->receiver =
        TransformEoaAddress(decoded_calldata.value()[2].GetString());
    swap_info->provider = "zeroex";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
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

    if (decoded_path->size() < 2) {
      return std::nullopt;
    }

    auto from_asset = decoded_path->front();
    auto to_asset = decoded_path->back();

    auto swap_info = mojom::SwapInfo::New();
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_coin = mojom::CoinType::ETH;
    swap_info->from_asset = from_asset;
    swap_info->from_amount = decoded_calldata.value()[1].GetString();
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = to_asset;
    swap_info->to_amount = decoded_calldata.value()[2].GetString();
    swap_info->receiver =
        TransformEoaAddress(decoded_calldata.value()[3].GetString());
    swap_info->provider = "zeroex";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
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

    if (decoded_calldata.value()[0].GetList().size() < 2) {
      return std::nullopt;
    }

    auto& from_asset = decoded_calldata.value()[0].GetList().front();
    if (!from_asset.is_string()) {
      return std::nullopt;
    }

    auto& to_asset = decoded_calldata.value()[0].GetList().back();
    if (!to_asset.is_string()) {
      return std::nullopt;
    }

    auto swap_info = mojom::SwapInfo::New();
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_coin = mojom::CoinType::ETH;
    swap_info->from_asset = from_asset.GetString();
    swap_info->from_amount = decoded_calldata.value()[1].GetString();
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = to_asset.GetString();
    swap_info->to_amount = decoded_calldata.value()[2].GetString();
    swap_info->receiver = "";  // unknown receiver
    swap_info->provider = "zeroex";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
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

    auto swap_info = mojom::SwapInfo::New();
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_coin = mojom::CoinType::ETH;
    swap_info->from_asset = decoded_calldata.value()[0].GetString();
    swap_info->from_amount = decoded_calldata.value()[2].GetString();
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = decoded_calldata.value()[1].GetString();
    swap_info->to_amount = decoded_calldata.value()[3].GetString();
    swap_info->receiver = "";  // unknown receiver
    swap_info->provider = "zeroex";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
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
    auto swap_info = mojom::SwapInfo::New();

    if (selector == kFillOtcOrderForEthSelector) {
      // The output of the swap is actually WETH but fillOtcOrderForEth()
      // automatically unwraps it to ETH. The buyToken is therefore the
      // 0x native asset contract.
      swap_info->from_asset = decoded_calldata.value()[1].GetString();
      swap_info->to_asset = kNativeEVMAssetContractAddress;
    } else if (selector == kFillOtcOrderWithEthSelector) {
      // The input of the swap is actually ETH but fillOtcOrderWithEth()
      // automatically wraps it to WETH. The sellToken is therefore the 0x
      // native asset contract.
      //
      // Clients are free to use the sellAmount extracted from calldata or
      // the value field of the swap transaction. The latter is more reliable
      // since OTC trades may include protocol fees payable in ETH that get
      // added to the sellAmount.
      swap_info->from_asset = kNativeEVMAssetContractAddress;
      swap_info->to_asset = decoded_calldata.value()[0].GetString();
    } else if (selector == kFillOtcOrderSelector) {
      swap_info->from_asset = decoded_calldata.value()[1].GetString();
      swap_info->to_asset = decoded_calldata.value()[0].GetString();
    }

    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_coin = mojom::CoinType::ETH;
    swap_info->from_amount = decoded_calldata.value()[3].GetString();
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_amount = decoded_calldata.value()[2].GetString();
    swap_info->receiver = "";  // unknown receiver
    swap_info->provider = "zeroex";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
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

    auto swap_info = mojom::SwapInfo::New();
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_coin = mojom::CoinType::ETH;
    swap_info->from_asset = kNativeEVMAssetContractAddress;
    swap_info->from_amount = decoded_calldata.value()[2].GetString();
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = decoded_calldata.value()[0].GetString();
    swap_info->to_amount = decoded_calldata.value()[3].GetString();
    swap_info->receiver =
        TransformEoaAddress(decoded_calldata.value()[1].GetString());
    swap_info->provider = "cowswap";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
  } else if (selector == kLiFiSwapTokensGeneric ||
             selector == kLiFiSwapTokensMultipleV3ERC20ToERC20 ||
             selector == kLiFiswapTokensMultipleV3ERC20ToNative ||
             selector == kLiFiSwapTokensMultipleV3NativeToERC20) {
    // The following block handles decoding of calldata for generic LiFi swap
    // orders of the following form.
    //
    // Function:
    // function(bytes32 transactionId,
    //          string integrator,
    //          string referrer,
    //          address receiver,
    //          uint256 minAmountOut,
    //          (address callTo,
    //           address approveTo,
    //           address sendingAssetId,
    //           address receivingAssetId,
    //           uint256 fromAmount,
    //           bytes callData,
    //           bool requiresDeposit)[] swapData)
    //
    // Ref (example):
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
    auto receiver = TransformEoaAddress(decoded.value()[3].GetString());

    // The swapData field is an array of tuples, each representing a swap fill
    // operation.
    auto swap_data = LiFiSwapDataDecode(decoded.value()[5].GetList());
    if (!swap_data) {
      return std::nullopt;
    }

    auto swap_info = mojom::SwapInfo::New();
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_coin = mojom::CoinType::ETH;
    swap_info->from_asset = swap_data->sending_asset_id;
    swap_info->from_amount = swap_data->from_amount;
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = swap_data->receiving_asset_id;
    swap_info->to_amount = min_amount_out;
    swap_info->receiver = receiver;
    swap_info->provider = "lifi";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
  } else if (selector == kLiFiSwapTokensSingleErc20ToErc20 ||
             selector == kLiFiSwapTokensSingleErc20ToNative ||
             selector == kLiFiSwapTokensSingleNativeToErc20 ||
             selector == kLiFiSwapTokensSingleV3NativeToERC20 ||
             selector == kLiFiSwapTokensSingleV3ERC20ToNative ||
             selector == kLiFiSwapTokensSingleV3ERC20ToERC20) {
    // The following block handles decoding of calldata for generic LiFi swap
    // orders of the following form.
    //
    // TXN: token → token
    // Function:
    // function(bytes32 transactionId,
    //          string integrator,
    //          string referrer,
    //          address receiver,
    //          uint256 minAmountOut,
    //          (address callTo,
    //           address approveTo,
    //           address sendingAssetId,
    //           address receivingAssetId,
    //           uint256 fromAmount,
    //           bytes callData,
    //           bool requiresDeposit) swapData)
    //
    // Ref (example):
    // https://github.com/lifinance/contracts/blob/0becf25cb5983e88d58636b6215b5a7aa1b267e0/src/Facets/GenericSwapFacetV3.sol#L25-L39
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
    auto receiver = TransformEoaAddress(decoded.value()[3].GetString());

    auto swap_data_list = base::Value::List();
    swap_data_list.Append(std::move(decoded.value()[5]));
    auto swap_data = LiFiSwapDataDecode(swap_data_list);
    if (!swap_data) {
      return std::nullopt;
    }

    auto swap_info = mojom::SwapInfo::New();
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_coin = mojom::CoinType::ETH;
    swap_info->from_asset = swap_data->sending_asset_id;
    swap_info->from_amount = swap_data->from_amount;
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = swap_data->receiving_asset_id;
    swap_info->to_amount = min_amount_out;
    swap_info->receiver = receiver;
    swap_info->provider = "lifi";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
  } else if (selector == kLiFiSwapAndStartBridgeTokensViaAcross ||
             selector == kLiFiSwapAndStartBridgeTokensViaAllBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaAmarok ||
             selector == kLiFiSwapAndStartBridgeTokensViaArbitrumBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaCBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaCelerCircleBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaCelerIM ||
             selector == kLiFiSwapAndStartBridgeTokensViaXDaiBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaHop ||
             selector == kLiFiSwapAndStartBridgeTokensViaHyphen ||
             selector == kLiFiSwapAndStartBridgeTokensViaLIFuel ||
             selector == kLiFiSwapAndStartBridgeTokensViaMayan ||
             selector == kLiFiSwapAndStartBridgeTokensViaMultichain ||
             selector == kLiFiSwapAndStartBridgeTokensViaOmniBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaOptimismBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaPolygonBridge ||
             selector == kLiFiSwapAndStartBridgeTokensViaSquid ||
             selector == kLiFiSwapAndStartBridgeTokensViaStargate ||
             selector == kLiFiSwapAndStartBridgeTokensViaStargateV2 ||
             selector == kLiFiSwapAndStartBridgeTokensViaSymbiosis ||
             selector == kLiFiSwapAndStartBridgeTokensViaThorSwap) {
    // The following block handles decoding of calldata for LiFi swap orders
    // that involve bridging assets to other chains.
    //
    // Function:
    // function((bytes32 transactionId,
    //           string bridge,
    //           string integrator,
    //           address referrer,
    //           address sendingAssetId,
    //           address receiver,
    //           uint256 minAmount,
    //           uint256 destinationChainId,
    //           bool hasSourceSwaps,
    //           bool hasDestinationCall) bridgeData,
    //          (address callTo,
    //           address approveTo,
    //           address sendingAssetId,
    //           address receivingAssetId,
    //           uint256 fromAmount,
    //           bytes callData,
    //           bool requiresDeposit)[] swapData)
    auto type =
        eth_abi::Tuple()
            .AddTupleType(MakeLiFiBridgeDataType())
            .AddTupleType(
                eth_abi::Array().SetArrayType(MakeLiFiSwapDataType()).build())
            .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    CHECK_EQ(2u, decoded.value().size());

    auto bridge_data = LiFiBridgeDataDecode(decoded.value()[0].GetList());
    if (!bridge_data) {
      return std::nullopt;
    }

    auto swap_data = LiFiSwapDataDecode(decoded.value()[1].GetList());
    if (!swap_data) {
      return std::nullopt;
    }

    auto swap_info = mojom::SwapInfo::New();
    swap_info->from_coin = mojom::CoinType::ETH;
    // from_chain_id is filled in by the caller.
    swap_info->from_asset = swap_data->sending_asset_id;
    swap_info->from_amount = swap_data->from_amount;
    swap_info->to_coin = bridge_data->destination_coin;
    swap_info->to_chain_id = bridge_data->destination_chain_id;
    // to_asset and to_amount cannot be reliably determined.
    swap_info->to_asset = "";
    swap_info->to_amount = "";
    swap_info->receiver = bridge_data->receiver;
    swap_info->provider = "lifi";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
  } else if (selector == kLiFiStartBridgeTokensViaAcross ||
             selector == kLiFiStartBridgeTokensViaAllBridge ||
             selector == kLiFiStartBridgeTokensViaAmarok ||
             selector == kLiFiStartBridgeTokensViaArbitrumBridge ||
             selector == kLiFiStartBridgeTokensViaCBridge ||
             selector == kLiFiStartBridgeTokensViaCelerCircleBridge ||
             selector == kLiFiStartBridgeTokensViaCelerIM ||
             selector == kLiFiStartBridgeTokensViaXDaiBridge ||
             selector == kLiFiStartBridgeTokensViaHop ||
             selector == kLiFiStartBridgeTokensViaHyphen ||
             selector == kLiFiStartBridgeTokensViaLIFuel ||
             selector == kLiFiStartBridgeTokensViaMayan ||
             selector == kLiFiStartBridgeTokensViaMultichain ||
             selector == kLiFiStartBridgeTokensViaOmniBridge ||
             selector == kLiFiStartBridgeTokensViaOptimismBridge ||
             selector == kLiFiStartBridgeTokensViaPolygonBridge ||
             selector == kLiFiStartBridgeTokensViaSquid ||
             selector == kLiFiStartBridgeTokensViaStargate ||
             selector == kLiFiStartBridgeTokensViaStargateV2 ||
             selector == kLiFiStartBridgeTokensViaSymbiosis ||
             selector == kLiFiStartBridgeTokensViaThorSwap) {
    // The following block handles decoding of calldata for LiFi orders that
    // involve bridging assets to other chains.
    //
    // Function:
    // function((bytes32 transactionId,
    //           string bridge,
    //           string integrator,
    //           address referrer,
    //           address sendingAssetId,
    //           address receiver,
    //           uint256 minAmount,
    //           uint256 destinationChainId,
    //           bool hasSourceSwaps,
    //           bool hasDestinationCall)) bridgeData)
    auto type = eth_abi::Tuple().AddTupleType(MakeLiFiBridgeDataType()).build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    CHECK_EQ(decoded.value().size(), 1u);

    auto bridge_data = LiFiBridgeDataDecode(decoded.value()[0].GetList());
    if (!bridge_data) {
      return std::nullopt;
    }

    auto swap_info = mojom::SwapInfo::New();
    swap_info->from_coin = mojom::CoinType::ETH;
    // from_chain_id is filled in by the caller.
    swap_info->from_asset = bridge_data->sending_asset_id;
    swap_info->from_amount = bridge_data->min_amount;
    swap_info->to_coin = bridge_data->destination_coin;
    swap_info->to_chain_id = bridge_data->destination_chain_id;
    // to_asset and to_amount are unavailable.
    swap_info->receiver = bridge_data->receiver;
    swap_info->provider = "lifi";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
  } else if (selector == kSquidFundAndRunMulticall) {
    // The following block handles decoding of calldata for Squid swap orders.
    //
    // Function:
    // fundAndRunMulticall(address token,
    //                     uint256 amount,
    //                     (bytes32 callType,
    //                      address target,
    //                      uint256 value,
    //                      bytes callData,
    //                      bytes payload)[] calls)
    //
    // Ref:
    // https://etherscan.io/address/0x9c01172bdbed2eea06e4e18ad534bf651c9089ea#code#F1#L88
    auto type = eth_abi::Tuple()
                    .AddTupleType(eth_abi::Address())
                    .AddTupleType(eth_abi::Uint(256))
                    .AddTupleType(
                        eth_abi::Array()
                            .SetArrayType(eth_abi::Tuple()
                                              .AddTupleType(eth_abi::Bytes(32))
                                              .AddTupleType(eth_abi::Address())
                                              .AddTupleType(eth_abi::Uint(256))
                                              .AddTupleType(eth_abi::Bytes())
                                              .AddTupleType(eth_abi::Bytes())
                                              .build())
                            .build())
                    .build();
    auto decoded = ABIDecode(type, calldata);
    if (!decoded) {
      return std::nullopt;
    }

    auto decoded_swap_data = SquidDecodeMulticall(decoded.value()[2].GetList());
    if (!decoded_swap_data) {
      return std::nullopt;
    }

    auto swap_info = mojom::SwapInfo::New();
    swap_info->from_coin = mojom::CoinType::ETH;
    // from_chain_id and to_chain_id are filled by caller.
    swap_info->from_asset = decoded.value()[0].GetString();
    swap_info->from_amount = decoded.value()[1].GetString();
    swap_info->to_coin = mojom::CoinType::ETH;
    swap_info->to_asset = decoded_swap_data->receiving_asset_id;
    swap_info->to_amount = decoded_swap_data->to_amount;
    swap_info->receiver = decoded_swap_data->receiver;
    swap_info->provider = "squid";

    return std::make_tuple(mojom::TransactionType::ETHSwap,
                           std::vector<std::string>{},
                           std::vector<std::string>{}, std::move(swap_info));
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
            decoded.value()[4].GetString()},
        nullptr);
  } else {
    return std::make_tuple(mojom::TransactionType::Other,
                           std::vector<std::string>(),
                           std::vector<std::string>(), nullptr);
  }
}

}  // namespace brave_wallet
