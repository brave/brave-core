/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/encoding_utils.h"

#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {

namespace {
// Prefix is added a public key to calculate
// blake2b hash.
constexpr char kSs58HashPrefix[] = "SS58PRE";
constexpr size_t kSs58HashChecksumSize = 2u;
// Prefix may be 1 or 2 bytes size and first bit in every byte
// points on prefix size, so 14 bits actually used for keeping prefix value.
constexpr uint16_t kSs58MaxPrefixValue = 16383u;
}  // namespace

Ss58Address::Ss58Address() = default;
Ss58Address::~Ss58Address() = default;
Ss58Address::Ss58Address(Ss58Address&& addr) = default;
Ss58Address& Ss58Address::operator=(Ss58Address&& addr) = default;

std::string Base58EncodeWithCheck(const std::vector<uint8_t>& bytes) {
  auto with_checksum = bytes;
  auto checksum = DoubleSHA256Hash(bytes);
  with_checksum.insert(with_checksum.end(), checksum.begin(),
                       checksum.begin() + 4);
  return Base58Encode(with_checksum);
}

bool Base58Decode(const std::string& str,
                  std::vector<uint8_t>* ret,
                  int len,
                  bool strict) {
  DCHECK(ret);
  ret->clear();
  return DecodeBase58(str, *ret, len) &&
         (!strict || static_cast<int>(ret->size()) == len);
}

std::optional<std::vector<uint8_t>> Base58Decode(const std::string& str,
                                                 int len,
                                                 bool strict) {
  std::vector<uint8_t> result;
  if (Base58Decode(str, &result, len, strict)) {
    return result;
  }
  return {};
}

std::string Base58Encode(base::span<const uint8_t> bytes) {
  return EncodeBase58(bytes);
}

// Reference implementation
// https://github.com/gear-tech/gear/blob/7d481fed39e7b0633ca9afeed8ce1b3cbb636f3e/utils/ss58/src/lib.rs#L295
std::optional<std::string> Ss58Address::Encode() {
  if (prefix > kSs58MaxPrefixValue) {
    return std::nullopt;
  }

  size_t offset = prefix < 64 ? 1 : 2;
  std::vector<uint8_t> buff(offset + kSs58PublicKeySize +
                            kSs58HashChecksumSize);
  auto output_span_writer = base::SpanWriter(base::span(buff));

  if (offset == 1) {
    output_span_writer.WriteU8BigEndian(prefix);
  } else {
    output_span_writer.WriteU8BigEndian(((prefix & 0b11111100) >> 2) |
                                        0b01000000);
    output_span_writer.WriteU8BigEndian((prefix >> 8) | ((prefix & 0b11) << 6));
  }

  output_span_writer.Write(public_key);
  DCHECK_EQ(output_span_writer.remaining(), kSs58HashChecksumSize);

  auto hash_prefix = base::byte_span_from_cstring(kSs58HashPrefix);
  auto hash = Blake2bHash<64>(
      {hash_prefix, base::span(buff).first(offset + kSs58PublicKeySize)});

  output_span_writer.Write(base::span(hash).first(kSs58HashChecksumSize));

  return Base58Encode(buff);
}

// Reference implementation
// https://github.com/gear-tech/gear/blob/7d481fed39e7b0633ca9afeed8ce1b3cbb636f3e/utils/ss58/src/lib.rs#L243
// static
std::optional<Ss58Address> Ss58Address::Decode(const std::string& str) {
  auto result =
      Base58Decode(str, kSs58HashChecksumSize + kSs58PublicKeySize + 2, false);
  if (!result ||
      result->size() < kSs58HashChecksumSize + kSs58PublicKeySize + 1) {
    return std::nullopt;
  }
  uint8_t offset = 0;
  auto result_span = base::span(*result);
  uint8_t address_type = result_span[0];
  uint16_t prefix = 0;
  if (address_type < 64) {
    offset = 1;
    prefix = address_type;
  } else if (address_type < 128) {
    offset = 2;
    uint8_t address_type_1 = result_span[1];
    uint8_t lower = ((address_type << 2) | (address_type_1 >> 6));
    uint8_t upper = address_type_1 & 0b00111111;
    prefix = lower | (upper << 8);
  } else {
    return std::nullopt;
  }

  if (result->size() != offset + kSs58PublicKeySize + kSs58HashChecksumSize) {
    return std::nullopt;
  }

  // Prepare input to calculate checksum
  auto hash_prefix = base::byte_span_from_cstring(kSs58HashPrefix);
  auto hash = Blake2bHash<64>(
      {hash_prefix, result_span.first(offset + kSs58PublicKeySize)});

  // Verify checksum
  if (base::span(hash).first(kSs58HashChecksumSize) !=
      result_span.last(kSs58HashChecksumSize)) {
    return std::nullopt;
  }

  // Prepare output
  Ss58Address result_addr;
  result_addr.prefix = prefix;
  base::span(result_addr.public_key)
      .copy_from(result_span.subspan(offset, kSs58PublicKeySize));
  return result_addr;
}

}  // namespace brave_wallet
