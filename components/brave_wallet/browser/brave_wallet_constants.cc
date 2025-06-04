/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"

#include <string>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/switches.h"

namespace brave_wallet {

std::optional<std::string> GetSardineNetworkName(std::string_view chain_id) {
  // key = chain_id, value = sardine_network_name
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      sardine_network_names({{mojom::kMainnetChainId, "ethereum"},
                             {mojom::kPolygonMainnetChainId, "ethereum"},
                             {mojom::kAvalancheMainnetChainId, "avalanche"},
                             {mojom::kFantomMainnetChainId, "fantom"},
                             {mojom::kSolanaMainnet, "solana"}});
  auto sardine_network_pair = sardine_network_names->find(chain_id);

  if (sardine_network_pair == sardine_network_names->end()) {
    // not found
    return std::nullopt;
  }
  return sardine_network_pair->second;
}

const base::flat_map<std::string, std::string>&
GetEthBalanceScannerContractAddresses() {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      contract_addresses(
          // Ref: https://github.com/brave/evm-scanner
          {{mojom::kArbitrumMainnetChainId,
            "0xfA542DD20c1997D6e8b24387D64CB8336197df3d"},
           {mojom::kAvalancheMainnetChainId,
            "0x827aa7e7C0C665df227Fae6dd155c0048fec6978"},
           {mojom::kBaseMainnetChainId,
            "0xF9164898C08f40DfB0999F94Bf9b9F73d66dfFeb"},
           {mojom::kBnbSmartChainMainnetChainId,
            "0x578E2574dDD2e609dDA7f6C8B2a90C540794B75e"},
           {mojom::kMainnetChainId,
            "0x667e61DB0997B59681C15E07376185aE24f754Db"},
           {mojom::kOptimismMainnetChainId,
            "0x2D1AacdEcd43Be64d82c14E9a6072A29dc804cAe"},
           {mojom::kPolygonMainnetChainId,
            "0x0B7Dd2c628a6Ee40153D89ce68bdA82d4840CD34"}});

  return *contract_addresses;
}

const std::vector<std::string>& GetEthSupportedNftInterfaces() {
  static base::NoDestructor<std::vector<std::string>> interfaces({
      kERC721InterfaceId,
      kERC1155InterfaceId,
  });

  return *interfaces;
}

std::string GetAssetRatioBaseURL() {
  std::string ratios_url =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kAssetRatioDevUrl);
  if (ratios_url.empty()) {
    return "https://ratios.wallet.brave.com";
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
    std::string_view chain_id) {
  // key = chain_id, value = allowance_holder_contract_address
  static base::NoDestructor<base::flat_map<std::string, std::string>>
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
      allowance_holder_addresses->find(chain_id);

  if (allowance_holder_address_pair == allowance_holder_addresses->end()) {
    // not found
    return std::nullopt;
  }

  return allowance_holder_address_pair->second;
}

}  // namespace brave_wallet
