/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_

#include <string>
#include <vector>
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"

namespace brave_wallet {

namespace erc20 {

// Allows transferring ERC20 tokens
bool Transfer(const std::string& to_address,
              uint256_t amount,
              std::string* data);
// Returns the balance of an  address
bool BalanceOf(const std::string& address, std::string* data);
// Approves the use of funds to an address
bool Approve(const std::string& spender_address,
             uint256_t amount,
             std::string* data);

}  // namespace erc20

namespace unstoppable_domains {

// Get mutiple record values mapped with keys of the target domain.
bool GetMany(const std::vector<std::string>& keys,
             const std::string& domain,
             std::string* data);

}  // namespace unstoppable_domains

namespace ens {

bool GetResolverAddress(const std::string& domain, std::string* data);
bool GetContentHashAddress(const std::string& domain, std::string* data);

}  // namespace ens

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
