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
#include "brave/third_party/argon2/src/src/blake2/blake2.h"
#include "brave/third_party/bitcoin-core/src/src/crypto/ripemd160.h"
#include "brave/third_party/ethash/src/include/ethash/keccak.h"
#include "crypto/hash.h"

namespace brave_wallet {
static_assert(kBlake2bMaxLength == BLAKE2B_OUTBYTES);
static_assert(kBlake2bPersonalizerLength == BLAKE2B_PERSONALBYTES);

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

std::string GetFunctionHash(std::string_view input) {
  return ToHex(GetFunctionHashBytes4(input));
}

eth_abi::Bytes4 GetFunctionHashBytes4(std::string_view input) {
  eth_abi::Bytes4 bytes_result;
  base::span(bytes_result)
      .copy_from(
          base::as_byte_span(KeccakHash(base::as_byte_span(input))).first<4>());
  return bytes_result;
}

eth_abi::Bytes32 Namehash(std::string_view name) {
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
  return crypto::hash::Sha256(crypto::hash::Sha256(input));
}

Ripemd160HashArray Hash160(base::span<const uint8_t> input) {
  Ripemd160HashArray result = {};

  CRIPEMD160()
      .Write(crypto::hash::Sha256(input).data(), crypto::hash::kSha256Size)
      .Finalize(result.data());

  return result;
}

std::vector<uint8_t> Blake2bHash(
    base::span<const uint8_t> payload,
    size_t length,
    std::optional<base::span<const uint8_t, kBlake2bPersonalizerLength>>
        personalizer /* = std::nullopt */) {
  CHECK_GT(length, 0u);
  CHECK_LE(length, kBlake2bMaxLength);

  blake2b_param param = {};
  param.digest_length = length;
  param.fanout = 1;
  param.depth = 1;
  if (personalizer) {
    base::span(param.personal).copy_from(*personalizer);
  }

  blake2b_state state = {};
  CHECK_EQ(0, blake2b_init_param(&state, &param));
  CHECK_EQ(0, blake2b_update(&state, payload.data(), payload.size()));

  std::vector<uint8_t> result(length, 0);
  CHECK_EQ(0, blake2b_final(&state, result.data(), result.size()));
  return result;
}

}  // namespace brave_wallet
