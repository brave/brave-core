/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_

#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

using CoingeckoIdsMap =
    base::flat_map<std::pair<std::string, std::string>, std::string>;

using TokenListMap =
    base::flat_map<std::string, std::vector<mojom::BlockchainTokenPtr>>;
using ChainList = std::vector<mojom::NetworkInfoPtr>;
using DappListMap =
    base::flat_map<std::string, std::vector<brave_wallet::mojom::DappPtr>>;
using OnRampTokensListMap =
    base::flat_map<mojom::OnRampProvider,
                   std::vector<mojom::BlockchainTokenPtr>>;
using OffRampTokensListMap =
    base::flat_map<mojom::OffRampProvider,
                   std::vector<mojom::BlockchainTokenPtr>>;
using RampTokenListMaps = std::pair<OnRampTokensListMap, OffRampTokensListMap>;

bool ParseTokenList(const std::string& json,
                    TokenListMap* token_list,
                    mojom::CoinType coin);
absl::optional<RampTokenListMaps> ParseRampTokenListMaps(
    const std::string& json);
absl::optional<std::vector<mojom::OnRampCurrency>> ParseOnRampCurrencyLists(
    const std::string& json);
std::string GetTokenListKey(mojom::CoinType coin, const std::string& chain_id);
bool ParseChainList(const std::string& json, ChainList* chain_list);
absl::optional<DappListMap> ParseDappLists(const std::string& json);
absl::optional<CoingeckoIdsMap> ParseCoingeckoIdsMap(const std::string& json);
absl::optional<std::vector<std::string>> ParseOfacAddressesList(
    const std::string& json);
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_
