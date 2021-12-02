/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RPC_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RPC_RESPONSE_PARSER_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {
bool ParseSingleStringResult(const std::string& json, std::string* result);
bool ParseResult(const std::string& json, base::Value* result);
bool ParseBoolResult(const std::string& json, bool* value);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RPC_RESPONSE_PARSER_H_
