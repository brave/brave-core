/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/gurl.h"

namespace brave_wallet {

using CoingeckoIdsMap =
    base::flat_map<std::pair<std::string, std::string>, std::string>;
using TokenListMap =
    base::flat_map<std::string, std::vector<mojom::BlockchainTokenPtr>>;
using ChainList = std::vector<mojom::NetworkInfoPtr>;
using PolkadotRpcUrlsMap = base::flat_map<std::string, std::vector<GURL>>;

bool ParseTokenList(const std::string& json, TokenListMap* token_list_map);
std::string GetTokenListKey(mojom::CoinType coin, const std::string& chain_id);
bool ParseChainList(const std::string& json, ChainList* chain_list);
std::optional<CoingeckoIdsMap> ParseCoingeckoIdsMap(const std::string& json);
std::optional<std::vector<std::string>> ParseRestrictedAddressesList(
    const std::string& json);
std::optional<PolkadotRpcUrlsMap> ParsePolkadotRpcUrls(const std::string& json);
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_
