/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_GAS_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_GAS_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

namespace eth {

std::optional<uint256_t> ScaleBaseFeePerGas(const std::string& value);

bool GetSuggested1559Fees(const std::vector<std::string>& base_fee_per_gas,
                          const std::vector<double>& gas_used_ratio,
                          const std::string& oldest_block,
                          const std::vector<std::vector<std::string>>& reward,
                          uint256_t* low_priority_fee,
                          uint256_t* avg_priority_fee,
                          uint256_t* high_priority_fee,
                          uint256_t* suggested_base_fee_per_gas);

}  // namespace eth

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_GAS_UTILS_H_
