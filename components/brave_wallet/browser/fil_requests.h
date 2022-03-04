/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_REQUESTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_REQUESTS_H_

#include <string>
#include "base/values.h"

namespace brave_wallet {

// Returns the balance of the account of given address.
std::string fil_getBalance(const std::string& address);
// Returns the transactions count of given address.
std::string fil_getTransactionCount(const std::string& address);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_REQUESTS_H_
