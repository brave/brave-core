/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CASHADDR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CASHADDR_H_

// Cashaddr is an address format inspired by bech32, created for
// eCash and Bitcoin Cash.
// https://github.com/Bitcoin-ABC/bitcoin-abc/blob/master/doc/standards/cashaddr.md

#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace brave_wallet::cashaddr {

enum AddressType : uint8_t { PUBKEY = 0, SCRIPT = 1 };

enum ChainType : uint8_t { MAIN = 0, TEST = 1 };

std::optional<std::string> PrefixFromChainType(const ChainType& chain_type);

struct AddressContent {
  AddressContent(AddressType type,
                 std::vector<uint8_t> hash,
                 ChainType chain_type);
  ~AddressContent();
  AddressContent(const AddressContent&);
  AddressContent& operator=(const AddressContent&);
  AddressContent(AddressContent&&);
  AddressContent& operator=(AddressContent&&);

  AddressType address_type;
  std::vector<uint8_t> hash;
  ChainType chain_type{ChainType::MAIN};
};

// Encode a cash address from a payload (script hash or public key
// hash) and prefix.
std::string EncodeCashAddress(const std::string& prefix,
                              const AddressContent& content);

// Decode a cash address.
// The address can be with or without prefix. If the prefix is present,
// it must match the expected prefix. If absent, the expected prefix is used
// when verifying the checksum.
std::optional<AddressContent> DecodeCashAddress(
    const std::string& address,
    const std::string& expected_prefix);

// Encode a cashaddr string.
std::string Encode(const std::string& prefix,
                   const std::vector<uint8_t>& values);

// Decode a cashaddr string. Returns (prefix, data).
// Empty prefix means failure.
std::optional<std::pair<std::string, std::vector<uint8_t>>> Decode(
    const std::string& str,
    const std::string& default_prefix);

}  // namespace brave_wallet::cashaddr

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CASHADDR_H_
