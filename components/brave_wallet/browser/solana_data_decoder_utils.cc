/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_data_decoder_utils.h"

#include <tuple>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

constexpr uint8_t kAuthorityTypeMax = 3;
constexpr size_t kMaxStringSize32Bit = 4294967291u;

absl::optional<uint8_t> DecodeUint8(const std::vector<uint8_t>& input,
                                    size_t& offset) {
  if (offset >= input.size() || input.size() - offset < sizeof(uint8_t)) {
    return absl::nullopt;
  }

  offset += sizeof(uint8_t);
  return input[offset - sizeof(uint8_t)];
}

absl::optional<std::string> DecodeUint8String(const std::vector<uint8_t>& input,
                                              size_t& offset) {
  auto ret = DecodeUint8(input, offset);
  if (!ret)
    return absl::nullopt;
  return base::NumberToString(*ret);
}

absl::optional<std::string> DecodeAuthorityTypeString(
    const std::vector<uint8_t>& input,
    size_t& offset) {
  auto ret = DecodeUint8(input, offset);
  if (ret && *ret <= kAuthorityTypeMax)
    return base::NumberToString(*ret);
  return absl::nullopt;
}

absl::optional<uint32_t> DecodeUint32(const std::vector<uint8_t>& input,
                                      size_t& offset) {
  if (offset >= input.size() || input.size() - offset < sizeof(uint32_t)) {
    return absl::nullopt;
  }

  // Read bytes in little endian order.
  base::span<const uint8_t> s =
      base::make_span(input.begin() + offset, sizeof(uint32_t));
  uint32_t uint32_le = *reinterpret_cast<const uint32_t*>(s.data());

  offset += sizeof(uint32_t);

#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return uint32_le;
#else
  return base::ByteSwap(uint32_le);
#endif
}

absl::optional<std::string> DecodeUint32String(
    const std::vector<uint8_t>& input,
    size_t& offset) {
  auto ret = DecodeUint32(input, offset);
  if (!ret)
    return absl::nullopt;
  return base::NumberToString(*ret);
}

absl::optional<uint64_t> DecodeUint64(const std::vector<uint8_t>& input,
                                      size_t& offset) {
  if (offset >= input.size() || input.size() - offset < sizeof(uint64_t)) {
    return absl::nullopt;
  }

  // Read bytes in little endian order.
  base::span<const uint8_t> s =
      base::make_span(input.begin() + offset, sizeof(uint64_t));
  uint64_t uint64_le = *reinterpret_cast<const uint64_t*>(s.data());

  offset += sizeof(uint64_t);

#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return uint64_le;
#else
  return base::ByteSwap(uint64_le);
#endif
}

absl::optional<std::string> DecodeUint64String(
    const std::vector<uint8_t>& input,
    size_t& offset) {
  auto ret = DecodeUint64(input, offset);
  if (!ret)
    return absl::nullopt;
  return base::NumberToString(*ret);
}

absl::optional<std::string> DecodePublicKey(const std::vector<uint8_t>& input,
                                            size_t& offset) {
  if (offset >= input.size() || input.size() - offset < kSolanaPubkeySize)
    return absl::nullopt;

  offset += kSolanaPubkeySize;
  return Base58Encode(std::vector<uint8_t>(
      input.begin() + offset - kSolanaPubkeySize, input.begin() + offset));
}

absl::optional<std::string> DecodeOptionalPublicKey(
    const std::vector<uint8_t>& input,
    size_t& offset) {
  if (offset == input.size()) {
    return absl::nullopt;
  }

  // First byte is 0 or 1 to indicate if public key is passed.
  // And the rest bytes are the actual public key.
  if (input[offset] == 0) {
    offset++;
    return "";  // No public key is passed.
  } else if (input[offset] == 1) {
    offset++;
    return DecodePublicKey(input, offset);
  } else {
    return absl::nullopt;
  }
}

// bincode::serialize uses two u32 together for the string length and a byte
// array for the actual strings. The first u32 represents the lower bytes of
// the length, the second represents the upper bytes. The upper bytes will have
// non-zero value only when the length exceeds the maximum of u32.
// We currently cap the length here to be the max size of std::string
// on 32 bit systems, it's safe to do so because currently we don't expect any
// valid cases would have strings larger than it.
absl::optional<std::string> DecodeString(const std::vector<uint8_t>& input,
                                         size_t& offset) {
  auto len_lower = DecodeUint32(input, offset);
  if (!len_lower || *len_lower > kMaxStringSize32Bit)
    return absl::nullopt;
  auto len_upper = DecodeUint32(input, offset);
  if (!len_upper || *len_upper != 0) {  // Non-zero means len exceeds u32 max.
    return absl::nullopt;
  }

  if (offset + *len_lower > input.size())
    return absl::nullopt;

  offset += *len_lower;
  return std::string(reinterpret_cast<const char*>(&input[offset - *len_lower]),
                     *len_lower);
}

// Expects a the bytes of a Borsh encoded Metadata struct (see
// https://docs.rs/spl-token-metadata/latest/spl_token_metadata/state/struct.Metadata.html)
// and returns the URI string in of the nested Data struct (see
// https://docs.rs/spl-token-metadata/latest/spl_token_metadata/state/struct.Data.html)
// as a GURL.
absl::optional<GURL> DecodeMetadataUri(const std::vector<uint8_t> data) {
  size_t offset = 0;
  offset = offset + /* Skip first byte for metadata.key */ 1 +
           /* Skip next 32 bytes for `metadata.update_authority` */ 32 +
           /* Skip next 32 bytes for `metadata.mint` */ 32;

  // Skip next field, metdata.data.name, a string
  // whose length is represented by a leading 32 bit integer
  auto length = DecodeUint32(data, offset);
  if (!length) {
    return absl::nullopt;
  }
  offset += static_cast<size_t>(*length);

  // Skip next field, `metdata.data.symbol`, a string
  // whose length is represented by a leading 32 bit integer
  length = DecodeUint32(data, offset);
  if (!length) {
    return absl::nullopt;
  }
  offset += static_cast<size_t>(*length);

  // Parse next field, metadata.data.uri, a string
  length = DecodeUint32(data, offset);
  if (!length) {
    return absl::nullopt;
  }
  std::string uri =
      std::string(reinterpret_cast<const char*>(&data[offset]), *length);
  return GURL(uri);
}

}  // namespace brave_wallet
