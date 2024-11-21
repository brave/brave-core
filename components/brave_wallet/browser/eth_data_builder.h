/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/types/optional_ref.h"
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
bool Transfer(std::string_view to_address, uint256_t amount, std::string* data);
// Returns the balance of an address
bool BalanceOf(std::string_view address, std::string* data);
// Approves the use of funds to an address
bool Approve(std::string_view spender_address,
             uint256_t amount,
             std::string* data);
bool Allowance(std::string_view owner_address,
               std::string_view spender_address,
               std::string* data);

}  // namespace erc20

namespace erc721 {

// Transfer ownership of an NFT.
bool TransferFromOrSafeTransferFrom(bool is_safe_transfer_from,
                                    std::string_view from,
                                    std::string_view to,
                                    uint256_t token_id,
                                    std::string* data);

// Find the owner of an NFT.
bool OwnerOf(uint256_t token_id, std::string* data);

// Get the URI of an NFT.
bool TokenUri(uint256_t token_id, std::string* data);

}  // namespace erc721

namespace erc1155 {

// Transfer the ownership of token from one address to another address.
bool SafeTransferFrom(std::string_view from_address,
                      std::string_view to_address,
                      uint256_t token_id,
                      uint256_t value,
                      std::string* data);

// Return the balance of an address for a token ID.
bool BalanceOf(std::string_view owner_address,
               uint256_t token_id,
               std::string* data);

// Get the URI of a token
bool Uri(uint256_t token_id, std::string* data);

}  // namespace erc1155

namespace erc165 {

// supportsInterface(bytes4)
inline constexpr uint8_t kSupportsInterfaceBytes4[] = {0x01, 0xff, 0xc9, 0xa7};

bool SupportsInterface(std::string_view interface_id, std::string* data);
std::vector<uint8_t> SupportsInterface(eth_abi::Span4 interface);

}  // namespace erc165

namespace unstoppable_domains {

// getMany(string[],uint256)
inline constexpr uint8_t kGetManySelector[] = {0x1b, 0xd8, 0xcc, 0x1a};

// Get mutiple record values mapped with keys of the target domain.
std::optional<std::string> GetMany(const std::vector<std::string>& keys,
                                   std::string_view domain);

std::vector<std::string> MakeEthLookupKeyList(std::string_view symbol,
                                              std::string_view chain_id);
std::vector<std::string> MakeSolLookupKeyList(std::string_view symbol);
std::vector<std::string> MakeFilLookupKeyList();

std::vector<uint8_t> GetWalletAddr(std::string_view domain,
                                   mojom::CoinType coin,
                                   std::string_view symbol,
                                   std::string_view chain_id);

}  // namespace unstoppable_domains

namespace ens {

std::string Resolver(std::string_view domain);

std::optional<std::vector<uint8_t>> DnsEncode(std::string_view dotted_name);

}  // namespace ens

namespace balance_scanner {

std::optional<std::string> TokensBalance(
    std::string_view owner_address,
    const std::vector<std::string>& contract_addresses);

}  // namespace balance_scanner

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_BUILDER_H_
