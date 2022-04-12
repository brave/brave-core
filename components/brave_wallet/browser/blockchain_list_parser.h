/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

using TokenListMap =
    base::flat_map<std::string, std::vector<mojom::BlockchainTokenPtr>>;
bool ParseTokenList(const std::string& json,
                    TokenListMap* token_list,
                    mojom::CoinType coin);
std::string GetTokenListKey(mojom::CoinType coin, const std::string& chain_id);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_LIST_PARSER_H_
