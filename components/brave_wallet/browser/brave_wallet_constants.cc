/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"

#include <string>

#include "base/command_line.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/types/optional_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/switches.h"

namespace brave_wallet {

std::optional<std::string_view> GetSardineNetworkName(
    std::string_view chain_id) {
  // key = chain_id, value = sardine_network_name
  static constexpr auto kSardineNetworkNames =
      base::MakeFixedFlatMap<std::string_view, std::string_view>(
          {{mojom::kMainnetChainId, "ethereum"},
           {mojom::kPolygonMainnetChainId, "ethereum"},
           {mojom::kAvalancheMainnetChainId, "avalanche"},
           {mojom::kFantomMainnetChainId, "fantom"},
           {mojom::kSolanaMainnet, "solana"}});

  return base::OptionalFromPtr(
      base::FindOrNull(kSardineNetworkNames, chain_id));
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

// See https://0x.org/docs/introduction/0x-cheat-sheet#allowanceholder-address
std::optional<std::string_view> GetZeroExAllowanceHolderAddress(
    std::string_view chain_id) {
  // key = chain_id, value = allowance_holder_contract_address
  static constexpr auto kAllowanceHolderAddress =
      base::MakeFixedFlatMap<std::string_view, std::string_view>(
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

  return base::OptionalFromPtr(
      base::FindOrNull(kAllowanceHolderAddress, chain_id));
}

}  // namespace brave_wallet
