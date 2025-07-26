/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CASHADDR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CASHADDR_H_

// Cashaddr is an address format inspired by bech32.

#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace brave_wallet::cashaddr {

/** Concatenate two vectors. */
template <typename V>
inline V Cat(V v1, const V& v2) {
  v1.reserve(v1.size() + v2.size());
  for (const auto& arg : v2) {
    v1.push_back(arg);
  }
  return v1;
}

/**
 * Convert from one power-of-2 number base to another.
 *
 * If padding is enabled, this always return true. If not, then it returns true
 * if all the bits of the input are encoded in the output.
 */
template <int frombits, int tobits, bool pad, typename O, typename I>
bool ConvertBits(const O& outfn, I it, I end) {
  size_t acc = 0;
  size_t bits = 0;
  constexpr size_t maxv = (1 << tobits) - 1;
  constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
  while (it != end) {
    acc = ((acc << frombits) | *it) & max_acc;
    bits += frombits;
    while (bits >= tobits) {
      bits -= tobits;
      outfn((acc >> bits) & maxv);
    }
    ++it;
  }

  if (pad) {
    if (bits) {
      outfn((acc << (tobits - bits)) & maxv);
    }
  } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
    return false;
  }

  return true;
}

const char MAINNET_PREFIX[] = "ecash";
const char TESTNET_PREFIX[] = "ectest";
const char REGTEST_PREFIX[] = "ecreg";

enum AddressType : uint8_t { PUBKEY = 0, SCRIPT = 1 };

enum ChainType : uint8_t { MAIN = 0, TEST = 1, REG = 2, UNKNOWN = 3 };

std::string PrefixFromChainType(const ChainType& chainType);

struct AddressContent {
  AddressContent(AddressType type,
                 std::vector<uint8_t> hash,
                 ChainType chainType = ChainType::MAIN);
  ~AddressContent();
  AddressContent(const AddressContent&);
  AddressContent& operator=(const AddressContent&);
  AddressContent(AddressContent&&);
  AddressContent& operator=(AddressContent&&);

  AddressType addressType;
  std::vector<uint8_t> hash;
  ChainType chainType{ChainType::MAIN};
};

/**
 * Encode a cash address from a payload (hash).
 *
 * @param prefix Cash address prefix to be used in the output address.
 * @param content AddressContent provides the payload (script hash or public key
 *                hash) and chain parameters (main chain, testnet, or regtest).
 * @return cash address
 */
std::string EncodeCashAddress(const std::string& prefix,
                              const AddressContent& content);

/**
 * Decode a cash address.
 *
 * The chainType is deduced from the prefix. If the prefix is not one of
 * "ecash", "ectest", or "ecreg", outContent.chainType will be set to
 * ChainType::UNKNOWN.
 *
 * @param address Cash address, with or without prefix.
 * @param expectedPrefix Expected prefix. This is used to verify the checksum
 *                       part of the address.
 * @param[out] outContent Address content.
 * @return true in case of success.
 */
std::optional<AddressContent> DecodeCashAddress(
    const std::string& address,
    const std::string& expectedPrefix);

/**
 * Encode a cashaddr string. Returns the empty string in case of failure.
 */
std::string Encode(const std::string& prefix,
                   const std::vector<uint8_t>& values);

/**
 * Decode a cashaddr string. Returns (prefix, data). Empty prefix means failure.
 */
std::pair<std::string, std::vector<uint8_t>> Decode(
    const std::string& str,
    const std::string& default_prefix);

}  // namespace brave_wallet::cashaddr

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CASHADDR_H_
