/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_CALL_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_CALL_DATA_BUILDER_H_

#include <string>
#include "base/values.h"

namespace brave_wallet {

namespace erc20 {

// Returns the balance of an  address
bool BalanceOf(const std::string& address, std::string* data);

}  // namespace erc20

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_CALL_DATA_BUILDER_H_
