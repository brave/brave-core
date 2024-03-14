/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/zcash_utils.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/f4_jumble.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/rust/lib.rs.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {

namespace {
constexpr size_t kPaddedHrpSize = 16;
constexpr size_t kPubKeyHashSize = 20;
constexpr size_t kPrefixSize = 2;

// https://zips.z.cash/zip-0316#encoding-of-unified-addresses
enum AddrType {
  kP2PKH = 0x00,
  kP2PSH = 0x01,
  kSapling = 0x02,
  kOrchard = 0x03,
  kMaxValue = kOrchard
};

using ParsedAddress = std::pair<AddrType, std::vector<uint8_t>>;

// https://btcinformation.org/en/developer-reference#compactsize-unsigned-integers
std::optional<uint64_t> ReadCompactSize(base::span<const uint8_t>& data) {
  uint64_t value;
  if (data.size() == 0) {
    return std::nullopt;
  }
  uint8_t type = data[0];
  if (data.size() > 0 && data[0] < 253) {
    value = type;
    data = data.subspan(1);
  } else if (type == 253 && data.size() >= 3) {
    value = base::numerics::U16FromBigEndian(data.subspan<1, 2u>());
    data = data.subspan(1 + 2);
  } else if (type <= 254 && data.size() >= 5) {
    value = base::numerics::U32FromBigEndian(data.subspan<1, 4u>());
    data = data.subspan(1 + 4);
  } else if (data.size() >= 9) {
    value = base::numerics::U64FromBigEndian(data.subspan<1, 8u>());
    data = data.subspan(1 + 8);
  } else {
    return std::nullopt;
  }
  return value;
}

std::optional<std::vector<ParsedAddress>> ParseUnifiedAddress(
    base::span<const uint8_t> dejumbled_data) {
  std::vector<ParsedAddress> result;
  while (!dejumbled_data.empty()) {
    auto type = ReadCompactSize(dejumbled_data);
    if (!type || *type > AddrType::kMaxValue) {
      return std::nullopt;
    }
    auto size = ReadCompactSize(dejumbled_data);
    if (!size || size == 0 || size > dejumbled_data.size()) {
      return std::nullopt;
    }
    ParsedAddress addr;
    addr.first = static_cast<AddrType>(*type);
    addr.second =
        std::vector(dejumbled_data.begin(), dejumbled_data.begin() + *size);
    result.push_back(std::move(addr));
    dejumbled_data = dejumbled_data.subspan(*size);
  }
  return result;
}

std::vector<uint8_t> GetNetworkPrefix(bool is_testnet) {
  return is_testnet ? std::vector<uint8_t>({0x1d, 0x25})
                    : std::vector<uint8_t>({0x1c, 0xb8});
}

}  // namespace

DecodedZCashAddress::DecodedZCashAddress() = default;
DecodedZCashAddress::~DecodedZCashAddress() = default;
DecodedZCashAddress::DecodedZCashAddress(const DecodedZCashAddress& other) =
    default;
DecodedZCashAddress& DecodedZCashAddress::operator=(
    const DecodedZCashAddress& other) = default;
DecodedZCashAddress::DecodedZCashAddress(DecodedZCashAddress&& other) = default;
DecodedZCashAddress& DecodedZCashAddress::operator=(
    DecodedZCashAddress&& other) = default;

bool IsUnifiedAddress(const std::string& address) {
  return address.starts_with("u1") || address.starts_with("utest1");
}

bool IsUnifiedTestnetAddress(const std::string& address) {
  return address.starts_with("utest1");
}

std::string PubkeyToTransparentAddress(base::span<const uint8_t> pubkey,
                                       bool testnet) {
  std::vector<uint8_t> result = GetNetworkPrefix(testnet);

  std::vector<uint8_t> data_part = Hash160(pubkey);
  result.insert(result.end(), data_part.begin(), data_part.end());
  return Base58EncodeWithCheck(result);
}

std::optional<std::string> PubkeyHashToTransparentAddress(
    base::span<const uint8_t> pubkey_hash,
    bool testnet) {
  // Hash160 output size is 20 bytes
  if (pubkey_hash.size() != 20u) {
    return std::nullopt;
  }
  std::vector<uint8_t> result = GetNetworkPrefix(testnet);
  result.insert(result.end(), pubkey_hash.begin(), pubkey_hash.end());
  return Base58EncodeWithCheck(result);
}

std::optional<DecodedZCashAddress> DecodeZCashAddress(
    const std::string& address) {
  std::vector<uint8_t> decode_result;
  if (!DecodeBase58Check(address, decode_result,
                         kPubKeyHashSize + kPrefixSize)) {
    return std::nullopt;
  }

  bool is_testnet = decode_result[0] == 0x1d && decode_result[1] == 0x25;
  bool is_mainnet = decode_result[0] == 0x1c && decode_result[1] == 0xb8;

  if (!is_testnet && !is_mainnet) {
    return std::nullopt;
  }

  std::vector<uint8_t> body(decode_result.begin() + kPrefixSize,
                            decode_result.end());

  DecodedZCashAddress result;
  result.pubkey_hash = body;
  result.testnet = is_testnet;

  return result;
}

std::vector<uint8_t> ZCashAddressToScriptPubkey(const std::string& address,
                                                bool testnet) {
  auto decoded_address = DecodeZCashAddress(address);
  if (!decoded_address) {
    return {};
  }

  if (testnet != decoded_address->testnet) {
    return {};
  }

  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  CHECK_EQ(decoded_address->pubkey_hash.size(), 20u);

  stream.Push8AsLE(0x76);                          // OP_DUP
  stream.Push8AsLE(0xa9);                          // OP_HASH
  stream.Push8AsLE(0x14);                          // hash size
  stream.PushBytes(decoded_address->pubkey_hash);  // hash
  stream.Push8AsLE(0x88);                          // OP_EQUALVERIFY
  stream.Push8AsLE(0xac);                          // OP_CHECKSIG

  return data;
}

// https://zips.z.cash/zip-0316#encoding-of-unified-addresses
std::optional<std::string> ExtractTransparentPart(
    const std::string& unified_address,
    bool is_testnet) {
  auto bech_result = decode_bech32(unified_address);
  if (!bech_result->is_ok()) {
    VLOG(0) << std::string(bech_result->error_message());
    return std::nullopt;
  }

  const auto& unwrapped = bech_result->unwrap();

  if (unwrapped.variant() != Bech32DecodeVariant::Bech32m) {
    return std::nullopt;
  }

  std::string expected_hrp = is_testnet ? "utest" : "u";
  if (unwrapped.hrp() != expected_hrp) {
    return std::nullopt;
  }

  auto reverted = RevertF4Jumble(unwrapped.data());
  // HRP with 16 bytes padding which is appended to the end of message
  if (!reverted || reverted->size() < kPaddedHrpSize) {
    return std::nullopt;
  }

  std::vector<uint8_t> padded_hrp(kPaddedHrpSize, 0);
  std::copy(reinterpret_cast<const uint8_t*>(expected_hrp.c_str()),
            reinterpret_cast<const uint8_t*>(expected_hrp.c_str()) +
                expected_hrp.length(),
            padded_hrp.begin());

  // Check that HRP is similar to the padded HRP
  if (!std::equal(padded_hrp.begin(), padded_hrp.end(),
                  reverted->end() - kPaddedHrpSize)) {
    return std::nullopt;
  }

  auto parts = ParseUnifiedAddress(
      base::make_span(*reverted).subspan(0, reverted->size() - kPaddedHrpSize));
  if (!parts.has_value()) {
    return std::nullopt;
  }

  for (const auto& part : parts.value()) {
    if (part.first == AddrType::kP2PKH) {
      return PubkeyHashToTransparentAddress(part.second, is_testnet);
    }
  }

  return std::nullopt;
}

}  // namespace brave_wallet
