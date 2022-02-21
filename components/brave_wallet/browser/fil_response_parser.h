/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Returns the balance of the account of given address.
bool ParseFilGetBalance(const std::string& json, std::string* hex_balance);
bool ParseFilGetTransactionCount(const std::string& json, uint64_t* count);
bool ParseFilEstimateGas(const std::string& json,
                         std::string* gas_premium,
                         std::string* gas_fee_cap,
                         uint64_t* gas_limit,
                         std::string* cid);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_
