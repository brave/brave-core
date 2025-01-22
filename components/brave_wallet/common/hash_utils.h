/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HASH_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HASH_UTILS_H_

#include <array>
#include <string>
#include <string_view>

#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "crypto/hash.h"

namespace brave_wallet {

inline constexpr size_t kKeccakHashLength = 32;
inline constexpr size_t kRipemd160HashLength = 20;
inline constexpr size_t kBlake2bPersonalizerLength = 16;
inline constexpr size_t kBlake2bMaxLength = 64;

using KeccakHashArray = std::array<uint8_t, kKeccakHashLength>;
using SHA256HashArray = std::array<uint8_t, crypto::hash::kSha256Size>;
using Ripemd160HashArray = std::array<uint8_t, kRipemd160HashLength>;

KeccakHashArray KeccakHash(base::span<const uint8_t> input);

// Returns the hex encoding of the first 4 bytes of the hash.
// For example: keccak('balanceOf(address)')
std::string GetFunctionHash(std::string_view input);
eth_abi::Bytes4 GetFunctionHashBytes4(std::string_view input);

// Implement namehash algorithm based on EIP-137 spec.
// Used for converting domain names in the classic format (ex: brave.crypto) to
// an ERC-721 token for ENS and Unstoppable Domains.
eth_abi::Bytes32 Namehash(std::string_view name);

// sha256(sha256(input))
SHA256HashArray DoubleSHA256Hash(base::span<const uint8_t> input);

// ripemd160(sha256(input))
Ripemd160HashArray Hash160(base::span<const uint8_t> input);

// blake2b-length(input, length, personalizer?)
std::vector<uint8_t> Blake2bHash(
    base::span<const uint8_t> payload,
    size_t length,
    std::optional<base::span<const uint8_t, kBlake2bPersonalizerLength>>
        personalizer = std::nullopt);
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HASH_UTILS_H_
