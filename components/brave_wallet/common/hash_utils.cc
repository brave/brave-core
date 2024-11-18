/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/hash_utils.h"

#include <algorithm>
#include <array>

#include "base/containers/adapters.h"
#include "base/containers/span.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/third_party/bitcoin-core/src/src/crypto/ripemd160.h"
#include "brave/third_party/ethash/src/include/ethash/keccak.h"
#include "crypto/sha2.h"

namespace brave_wallet {
namespace {
std::array<uint8_t, 64> ConcatArrays(const std::array<uint8_t, 32>& arr1,
                                     const std::array<uint8_t, 32>& arr2) {
  std::array<uint8_t, 64> result;
  base::ranges::copy(arr1, result.begin());
  base::ranges::copy(arr2, result.begin() + 32);
  return result;
}
}  // namespace

KeccakHashArray KeccakHash(base::span<const uint8_t> input) {
  auto hash = ethash_keccak256(input.data(), input.size());
  KeccakHashArray result;
  static_assert(sizeof(result) == sizeof(hash.bytes));
  base::ranges::copy(hash.bytes, result.begin());
  return result;
}

std::string GetFunctionHash(const std::string& input) {
  return ToHex(GetFunctionHashBytes4(input));
}

eth_abi::Bytes4 GetFunctionHashBytes4(const std::string& input) {
  eth_abi::Bytes4 bytes_result;
  base::span(bytes_result)
      .copy_from(
          base::as_byte_span(KeccakHash(base::as_byte_span(input))).first<4>());
  return bytes_result;
}

eth_abi::Bytes32 Namehash(const std::string& name) {
  eth_abi::Bytes32 hash = {};
  auto labels = SplitStringPiece(name, ".", base::KEEP_WHITESPACE,
                                 base::SPLIT_WANT_NONEMPTY);

  for (const auto& label : base::Reversed(labels)) {
    auto label_hash = KeccakHash(base::as_byte_span(label));
    hash = KeccakHash(ConcatArrays(hash, label_hash));
  }
  return hash;
}

SHA256HashArray DoubleSHA256Hash(base::span<const uint8_t> input) {
  return crypto::SHA256Hash(crypto::SHA256Hash(input));
}

Ripemd160HashArray Hash160(base::span<const uint8_t> input) {
  Ripemd160HashArray result = {};

  CRIPEMD160()
      .Write(crypto::SHA256Hash(input).data(), crypto::kSHA256Length)
      .Finalize(result.data());

  return result;
}

}  // namespace brave_wallet
