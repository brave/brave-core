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
           {mojom::kBnbSmartChainMainnetChainId,
            "0x53242a975aa7c607e17138b0e0231162e3e68593"},
           {mojom::kOptimismMainnetChainId,
            "0x9e5076DF494FC949aBc4461F4E57592B81517D81"},
           {mojom::kArbitrumMainnetChainId,
            "0xa3e7eb35e779f261ca604138d41d0258e995e97b"}});

  return *contract_addresses;
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

// See https://api-docs.ankr.com/reference/post_ankr-getaccountbalance-1
// for full list.
const base::flat_map<std::string, std::string>& GetAnkrBlockchains() {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      blockchains({{mojom::kArbitrumMainnetChainId, "arbitrum"},
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

  return *blockchains;
}

// See https://0x.org/docs/introduction/0x-cheat-sheet#allowanceholder-address
std::optional<std::string> GetZeroExAllowanceHolderAddress(
    const std::string& chain_id) {
  // key = chain_id, value = allowance_holder_contract_address
  static base::NoDestructor<std::map<std::string, std::string>>
      allowance_holder_addresses(
          {{mojom::kMainnetChainId, kZeroExAllowanceHolderCancun},
           {mojom::kArbitrumMainnetChainId, kZeroExAllowanceHolderCancun},
           {mojom::kAvalancheMainnetChainId, kZeroExAllowanceHolderShanghai},
           {mojom::kBaseMainnetChainId, kZeroExAllowanceHolderCancun},
           {mojom::kBlastMainnetChainId, kZeroExAllowanceHolderCancun},
           {mojom::kBnbSmartChainMainnetChainId, kZeroExAllowanceHolderCancun},
           {mojom::kLineaChainId, kZeroExAllowanceHolderLondon},
           {mojom::kOptimismMainnetChainId, kZeroExAllowanceHolderCancun},
           {mojom::kPolygonMainnetChainId, kZeroExAllowanceHolderCancun},
           {mojom::kScrollChainId, kZeroExAllowanceHolderShanghai}});

  auto allowance_holder_address_pair =
      allowance_holder_addresses->find(chain_id.c_str());

  if (allowance_holder_address_pair == allowance_holder_addresses->end()) {
    // not found
    return std::nullopt;
  }

  return allowance_holder_address_pair->second;
}

}  // namespace brave_wallet
