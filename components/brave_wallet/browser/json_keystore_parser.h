/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_KEYSTORE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_KEYSTORE_PARSER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"

namespace brave_wallet {

// https://ethereum.org/en/developers/docs/data-structures-and-encoding/web3-secret-storage/
// https://github.com/web3/web3.js/blob/ae994346a656688b8e9b907e1ab4731be8c5736e/packages/web3-eth-accounts/src/account.ts#L769
std::optional<std::vector<uint8_t>> DecryptPrivateKeyFromJsonKeystore(
    const std::string& password,
    const base::Value::Dict& json);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_KEYSTORE_PARSER_H_
