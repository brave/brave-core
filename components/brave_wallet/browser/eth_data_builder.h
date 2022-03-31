/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_

#include <string>
#include <vector>
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

namespace erc20 {

// Allows transferring ERC20 tokens
bool Transfer(const std::string& to_address,
              uint256_t amount,
              std::string* data);
// Returns the balance of an address
bool BalanceOf(const std::string& address, std::string* data);
// Approves the use of funds to an address
bool Approve(const std::string& spender_address,
             uint256_t amount,
             std::string* data);
bool Allowance(const std::string& owner_address,
               const std::string& spender_address,
               std::string* data);

}  // namespace erc20

namespace erc721 {

// Transfer ownership of an NFT.
bool TransferFromOrSafeTransferFrom(bool is_safe_transfer_from,
                                    const std::string& from,
                                    const std::string& to,
                                    uint256_t token_id,
                                    std::string* data);

// Find the owner of an NFT.
bool OwnerOf(uint256_t token_id, std::string* data);

}  // namespace erc721

namespace erc165 {

bool SupportsInterface(const std::string& interface_id, std::string* data);

}  // namespace erc165

namespace unstoppable_domains {

// Get mutiple record values mapped with keys of the target domain.
bool GetMany(const std::vector<std::string>& keys,
             const std::string& domain,
             std::string* data);

// Get the value of the key for the target domain.
bool Get(const std::string& key, const std::string& domain, std::string* data);

}  // namespace unstoppable_domains

namespace ens {

bool Resolver(const std::string& domain, std::string* data);
bool ContentHash(const std::string& domain, std::string* data);

// Get Ethereum address from an ENS name.
bool Addr(const std::string& domain, std::string* data);

}  // namespace ens

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
