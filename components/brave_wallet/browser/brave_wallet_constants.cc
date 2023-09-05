/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"

#include <map>
#include <string>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/switches.h"

namespace brave_wallet {

const std::string GetSardineNetworkName(const std::string& chain_id) {
  // key = chain_id, value = sardine_network_name
  static std::map<std::string, std::string> sardine_network_names = {
      {mojom::kMainnetChainId, "ethereum"},
      {mojom::kPolygonMainnetChainId, "ethereum"},
      {mojom::kAvalancheMainnetChainId, "avalanche"},
      {mojom::kFantomMainnetChainId, "fantom"},
      {mojom::kSolanaMainnet, "solana"}};
  auto sardine_network_pair = sardine_network_names.find(chain_id.c_str());

  if (sardine_network_pair == sardine_network_names.end()) {
    // not found
    return "";
  } else {
    return sardine_network_pair->second;
  }
}

const base::flat_map<std::string, std::string>& GetInfuraChainEndpoints() {
  static base::NoDestructor<base::flat_map<std::string, std::string>> endpoints(
      {{brave_wallet::mojom::kPolygonMainnetChainId,
        "https://mainnet-polygon.brave.com/"},
       {brave_wallet::mojom::kOptimismMainnetChainId,
        "https://mainnet-optimism.brave.com/"},
       {brave_wallet::mojom::kAuroraMainnetChainId,
        "https://mainnet-aurora.brave.com/"},
       {brave_wallet::mojom::kAvalancheMainnetChainId,
        "https://mainnet-avalanche.wallet.brave.com/"}});

  return *endpoints;
}

const base::flat_map<std::string, std::string>&
GetEthBalanceScannerContractAddresses() {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      contract_addresses(
          // Mainnet, Polygon, and Avalanche conctract addresses pulled from
          // https://github.com/MyCryptoHQ/eth-scan
          {{mojom::kMainnetChainId,
            "0x08A8fDBddc160A7d5b957256b903dCAb1aE512C5"},
           {mojom::kPolygonMainnetChainId,
            "0x08A8fDBddc160A7d5b957256b903dCAb1aE512C5"},
           {mojom::kAvalancheMainnetChainId,
            "0x08A8fDBddc160A7d5b957256b903dCAb1aE512C5"},
           // BSC, Optimism, and Arbitrum contract addresses pulled from
           // https://github.com/onyb/x/blob/75800edce88688dcfe59dd6b4a664087862369bb/core/evm/scanner/balances/EVMScanner.ts
           {mojom::kBinanceSmartChainMainnetChainId,
            "0x53242a975aa7c607e17138b0e0231162e3e68593"},
           {mojom::kOptimismMainnetChainId,
            "0x9e5076DF494FC949aBc4461F4E57592B81517D81"},
           {mojom::kArbitrumMainnetChainId,
            "0xa3e7eb35e779f261ca604138d41d0258e995e97b"}});

  return *contract_addresses;
}

bool HasJupiterFeesForTokenMint(const std::string& mint) {
  static std::vector<std::string> mints(
      {"So11111111111111111111111111111111111111112",     // wSOL
       "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",    // USDC
       "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",    // USDT
       "7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs",    // WETH (Wormhole)
       "2FPyTwcZLUg1MDrwsyoP4D6s1tM7hAkHYRjkNb5w6Pxk",    // ETH (Sollet)
       "9n4nbM75f5Ui33ZbPYXn59EwSgE8CGsHtAeTH5YFeJ9E",    // BTC (Sollet)
       "qfnqNqs3nCAHjnyCgLRDbBtq4p2MtHZxw8YjSyYhPoL",     // wWBTC (Wormhole)
       "7dHbWXmci3dT8UFYWYZweBLXgycu7Y3iL6trKn1Y7ARj",    // stSOL
       "mSoLzYCxHdYgdzU16g5QSh3i5K3z3KZK7ytfqcJm7So",     // mSOL
       "FYpdBuyAHSbdaAyD1sKkxyLWbAP8uUW9h6uvdhK74ij1"});  // DAI

  return base::Contains(mints, mint);
}

const std::vector<std::string>& GetEthSupportedNftInterfaces() {
  static base::NoDestructor<std::vector<std::string>> interfaces({
      kERC721InterfaceId,
      kERC1155InterfaceId,
  });

  return *interfaces;
}

const std::string GetAssetRatioBaseURL() {
  std::string ratios_url =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kAssetRatioDevUrl);
  if (ratios_url.empty()) {
    ratios_url = "https://ratios.wallet.brave.com";
  }

  return ratios_url;
}

const base::flat_map<std::string, std::string>& GetAnkrBlockchains() {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      blockchains({{mojom::kArbitrumMainnetChainId, "arbitrum"},
                   {mojom::kAvalancheMainnetChainId, "avalanche"},
                   {mojom::kBaseMainnetChainId, "base"},
                   {mojom::kBinanceSmartChainMainnetChainId, "bsc"},
                   {mojom::kMainnetChainId, "eth"},
                   {mojom::kFantomMainnetChainId, "fantom"},
                   {mojom::kFlareMainnetChainId, "flare"},
                   {mojom::kGnosisChainId, "gnosis"},
                   {mojom::kOptimismMainnetChainId, "optimism"},
                   {mojom::kPolygonMainnetChainId, "polygon"},
                   {mojom::kPolygonZKEVMChainId, "polygon_zkevm"},
                   {mojom::kRolluxMainnetChainId, "rollux"},
                   {mojom::kSyscoinMainnetChainId, "syscoin"},
                   {mojom::kZkSyncEraChainId, "zksync_era"},
                   {mojom::kGoerliChainId, "eth_goerli"}});

  return *blockchains;
}

}  // namespace brave_wallet
