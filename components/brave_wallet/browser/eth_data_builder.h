/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/fil_address.h"

namespace brave_wallet {

namespace filforwarder {

// Allows to forward funds from FEVM account fo FVM account
// https://github.com/lotus-web3/FilForwarder/blob/main/contracts/FilForwarder.sol
std::optional<std::vector<uint8_t>> Forward(const FilAddress& fil_address);

}  // namespace filforwarder

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

// Get the URI of an NFT.
bool TokenUri(uint256_t token_id, std::string* data);

}  // namespace erc721

namespace erc1155 {

// Transfer the ownership of token from one address to another address.
bool SafeTransferFrom(const std::string& from_address,
                      const std::string& to_address,
                      uint256_t token_id,
                      uint256_t value,
                      std::string* data);

// Return the balance of an address for a token ID.
bool BalanceOf(const std::string& owner_address,
               uint256_t token_id,
               std::string* data);

// Get the URI of a token
bool Uri(uint256_t token_id, std::string* data);

}  // namespace erc1155

namespace erc165 {

// supportsInterface(bytes4)
inline constexpr uint8_t kSupportsInterfaceBytes4[] = {0x01, 0xff, 0xc9, 0xa7};

bool SupportsInterface(const std::string& interface_id, std::string* data);
std::vector<uint8_t> SupportsInterface(eth_abi::Span4 interface);

}  // namespace erc165

namespace unstoppable_domains {

// getMany(string[],uint256)
inline constexpr uint8_t kGetManySelector[] = {0x1b, 0xd8, 0xcc, 0x1a};

// Get mutiple record values mapped with keys of the target domain.
std::optional<std::string> GetMany(const std::vector<std::string>& keys,
                                   const std::string& domain);

std::vector<std::string> MakeEthLookupKeyList(const std::string& symbol,
                                              const std::string& chain_id);
std::vector<std::string> MakeSolLookupKeyList(const std::string& symbol);
std::vector<std::string> MakeFilLookupKeyList();

std::vector<uint8_t> GetWalletAddr(const std::string& domain,
                                   mojom::CoinType coin,
                                   const std::string& symbol,
                                   const std::string& chain_id);

}  // namespace unstoppable_domains

namespace ens {

std::string Resolver(const std::string& domain);

std::optional<std::vector<uint8_t>> DnsEncode(const std::string& dotted_name);

}  // namespace ens

namespace balance_scanner {

std::optional<std::string> TokensBalance(
    const std::string& owner_address,
    const std::vector<std::string>& contract_addresses);

}  // namespace balance_scanner

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
