/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

std::optional<std::tuple<mojom::TransactionType,     // tx_type
                         std::vector<std::string>,   // tx_params
                         std::vector<std::string>>>  // tx_args
GetTransactionInfoFromData(const std::vector<uint8_t>& data);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_
