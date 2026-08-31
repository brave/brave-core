/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_

#include <array>
#include <string>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "brave/brave_domains/urls.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/grit/brave_components_strings.h"

namespace brave_wallet {

// Re-export from brave_domains
using brave_domains::GetGate3URL;

inline constexpr char kBraveServicesKeyHeader[] = "x-brave-key";

inline constexpr uint256_t kDefaultSendEthGasLimit = 21000;
inline constexpr uint256_t kDefaultERC20TransferGasLimit = 300000;
inline constexpr uint256_t kDefaultERC721TransferGasLimit = 800000;
inline constexpr uint256_t kDefaultERC20ApproveGasLimit = 300000;

inline constexpr int32_t kAutoLockMinutesMin = 1;
inline constexpr int32_t kAutoLockMinutesMax = 10080;

inline constexpr int32_t kAssetDiscoveryMinutesPerRequest = 1;
inline constexpr size_t kBalanceScannerBatchSize = 4000;

inline constexpr char kWalletBaseDirectory[] = "BraveWallet";
inline constexpr char kImageSourceHost[] = "erc-token-images";
inline constexpr char kRampBaseUrl[] = "https://app.ramp.network";
inline constexpr char kOffRampEnabledFlows[] = "OFFRAMP";
inline constexpr char kOnRampEnabledFlows[] = "ONRAMP";
inline constexpr char kOnRampID[] = "8yxja8782as5essk2myz3bmh4az6gpq4nte9n2gf";
inline constexpr char kOffRampID[] = "y57zqta99ohs7o2paf4ak6vpfb7wf8ubj9krwtwe";
inline constexpr char kTransakURL[] = "https://global.transak.com/";
inline constexpr char kTransakApiKey[] = "985d14f0-4cf5-4a4c-8917-78107620d3b7";
inline constexpr char kCoinbaseURL[] = "https://pay.coinbase.com";
inline constexpr char kCoinbaseAppId[] = "8072ff71-8469-4fef-9404-7c905e2359c9";
inline constexpr size_t kSimpleHashMaxBatchSize = 50;



// 0x swap constants
inline constexpr char kZeroExBaseAPIURL[] = "https://api.0x.wallet.brave.com";
inline constexpr char kEVMFeeRecipient[] =
    "0xbd9420A98a7Bd6B89765e5715e169481602D9c3d";
inline constexpr char kZeroExAllowanceHolderCancun[] =
    "0x0000000000001fF3684f28c67538d4D072C22734";
inline constexpr char kZeroExAllowanceHolderShanghai[] =
    "0x0000000000005E88410CcDFaDe4a5EfaE4b49562";
inline constexpr char kZeroExAllowanceHolderLondon[] =
    "0x000000000000175a8b9bC6d539B3708EEd92EA6c";
inline constexpr char kZeroExAPIVersionHeader[] = "0x-version";
inline constexpr char kZeroExAPIVersion[] = "v2";

// Jupiter swap constants
inline constexpr char kJupiterBaseAPIURL[] =
    "https://jupiter-lite.wallet.brave.com";
inline constexpr char kJupiterReferralKey[] =
    "7yke2kxg6ewNsun61qBkdsLdxuXcUiB8CMB47Zv39Aoy";
inline constexpr char kJupiterReferralProgram[] =
    "REFER4ZgmyYx9c6He5XfaTMiGfdLwRnkV4RPp9t9iF3";
inline constexpr char kJupiterReferralProgramHeader[] = "referral_ata";
inline constexpr char kWrappedSolanaMintAddress[] =
    "So11111111111111111111111111111111111111112";

// Blowfish simulations constants
inline constexpr char kBlowfishBaseAPIURL[] =
    "https://blowfish.wallet.brave.com";
inline constexpr char kBlowfishAPIVersionHeader[] = "X-Api-Version";
inline constexpr char kBlowfishAPIVersion[] = "2023-06-05";

// 0x // Squid common constants
inline constexpr char kNativeEVMAssetContractAddress[] =
    "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";

inline constexpr int64_t kBlockTrackerDefaultTimeInSeconds = 20;
inline constexpr int64_t kLogTrackerDefaultTimeInSeconds = 20;
inline constexpr int64_t kSolanaBlockTrackerTimeInSeconds = 2;

// Ankr constants
inline constexpr char kAnkrAdvancedAPIBaseURL[] =
    "https://multichain.ankr.wallet.brave.com/";

// Unstoppable domains record key for ethereum address.
inline constexpr char kCryptoEthAddressKey[] = "crypto.ETH.address";
// Unstoppable domains record key for solana address.
inline constexpr char kCryptoSolAddressKey[] = "crypto.SOL.address";
// Unstoppable domains record key for filecoin address.
inline constexpr char kCryptoFilAddressKey[] = "crypto.FIL.address";

// ERC-165 identifier for ERC721 interface.
inline constexpr char kERC1155InterfaceId[] = "0xd9b67a26";
inline constexpr char kERC721InterfaceId[] = "0x80ac58cd";

inline constexpr char kEthereumBlockTagEarliest[] = "earliest";
inline constexpr char kEthereumBlockTagLatest[] = "latest";

inline constexpr char kBitcoinTestnetRpcEndpoint[] =
    "https://blockstream.info/testnet/api/";

inline constexpr char kMeldRpcEndpoint[] = "https://api-meld.wallet.brave.com";
inline constexpr char kMeldRpcVersionHeader[] = "Meld-Version";
inline constexpr char kMeldRpcVersion[] = "2023-05-26";

inline constexpr auto kEthBalanceScannerContractAddresses =
    base::MakeFixedFlatMap<std::string_view, std::string_view>(
        // Ref: https://github.com/brave/evm-scanner
        {{mojom::kArbitrumMainnetChainId,
          "0xfA542DD20c1997D6e8b24387D64CB8336197df3d"},
         {mojom::kAvalancheMainnetChainId,
          "0x827aa7e7C0C665df227Fae6dd155c0048fec6978"},
         {mojom::kBaseMainnetChainId,
          "0xF9164898C08f40DfB0999F94Bf9b9F73d66dfFeb"},
         {mojom::kBnbSmartChainMainnetChainId,
          "0x578E2574dDD2e609dDA7f6C8B2a90C540794B75e"},
         {mojom::kMainnetChainId, "0x667e61DB0997B59681C15E07376185aE24f754Db"},
         {mojom::kOptimismMainnetChainId,
          "0x2D1AacdEcd43Be64d82c14E9a6072A29dc804cAe"},
         {mojom::kPolygonMainnetChainId,
          "0x0B7Dd2c628a6Ee40153D89ce68bdA82d4840CD34"}});

// See https://api-docs.ankr.com/reference/post_ankr-getaccountbalance-1
// for full list.
inline constexpr auto kAnkrBlockchains =
    base::MakeFixedFlatMap<std::string_view, std::string_view>(
        {{mojom::kArbitrumMainnetChainId, "arbitrum"},
         {mojom::kAvalancheMainnetChainId, "avalanche"},
         {mojom::kBaseMainnetChainId, "base"},
         {mojom::kBnbSmartChainMainnetChainId, "bsc"},
         {mojom::kMainnetChainId, "eth"},
         {mojom::kFantomMainnetChainId, "fantom"},
         {mojom::kFlareMainnetChainId, "flare"},
         {mojom::kGnosisChainId, "gnosis"},
         {mojom::kOptimismMainnetChainId, "optimism"},
         {mojom::kPolygonMainnetChainId, "polygon"},
         {mojom::kPolygonZKEVMChainId, "polygon_zkevm"},
         {mojom::kRolluxMainnetChainId, "rollux"},
         {mojom::kSyscoinMainnetChainId, "syscoin"},
         {mojom::kZkSyncEraChainId, "zksync_era"}});

inline constexpr auto kEthSupportedNftInterfaces =
    std::to_array<std::string_view>({
        kERC721InterfaceId,
        kERC1155InterfaceId,
    });

// https://docs.rs/solana-program/1.18.10/src/solana_program/clock.rs.html#129-131
inline constexpr int kSolanaValidBlockHeightThreshold = 150;

// Returns the URL for the Ratios service.
std::string GetAssetRatioBaseURL();

std::optional<std::string_view> GetZeroExAllowanceHolderAddress(
    std::string_view chain_id);

std::optional<mojom::CoinType> GetCoinTypeFromString(const std::string& coin);
std::optional<std::string> GetStringFromCoinType(mojom::CoinType coin_type);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_
