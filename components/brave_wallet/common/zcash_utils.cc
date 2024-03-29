/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/zcash_utils.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/big_endian.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/f4_jumble.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/rust/lib.rs.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "brave/third_party/bitcoin-core/src/src/bech32.h"
#include "brave/third_party/bitcoin-core/src/src/util/strencodings.h"

namespace brave_wallet {

namespace {
constexpr char kTestnetHRP[] = "utest";
constexpr char kMainnetHRP[] = "u";
constexpr size_t kPaddedHrpSize = 16;
constexpr size_t kPubKeyHashSize = 20;
constexpr size_t kPrefixSize = 2;


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
    uint16_t val = 0;
    base::ReadBigEndian(&data[1], &val);
    value = val;
    data = data.subspan(1 + 2);
  } else if (type <= 254 && data.size() >= 5) {
    uint32_t val = 0;
    base::ReadBigEndian(&data[1], &val);
    value = val;
    data = data.subspan(1 + 4);
  } else if (data.size() >= 9) {
    uint64_t val = 0;
    base::ReadBigEndian(&data[1], &val);
    value = val;
    data = data.subspan(1 + 8);
  } else {
    return std::nullopt;
  }
  return value;
}

std::vector<uint8_t> SerializeUnifiedAddress(std::vector<ParsedAddress> parts) {
  std::vector<uint8_t> result;
  BtcLikeSerializerStream stream(&result);
  for (const auto& part : parts) {
    stream.PushCompactSize(part.first);
    stream.PushSizeAndBytes(part.second);
  }
  return result;
}

std::optional<std::vector<ParsedAddress>> ParseUnifiedAddressBody(
    base::span<const uint8_t> dejumbled_data) {
  std::vector<ParsedAddress> result;
  while (!dejumbled_data.empty()) {
    auto type = ReadCompactSize(dejumbled_data);
    if (!type || *type > ZCashAddrType::kMaxValue) {
      return std::nullopt;
    }
    auto size = ReadCompactSize(dejumbled_data);
    if (!size || size == 0 || size > dejumbled_data.size()) {
      return std::nullopt;
    }
    ParsedAddress addr;
    addr.first = static_cast<ZCashAddrType>(*type);
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
  return address.starts_with(base::StrCat({kTestnetHRP, "1"})) ||
         address.starts_with(base::StrCat({kMainnetHRP, "1"}));
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

std::optional<std::vector<ParsedAddress>> ExtractParsedAddresses(
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

  std::string expected_hrp = is_testnet ? kTestnetHRP : kMainnetHRP;
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

  auto parts = ParseUnifiedAddressBody(
      base::make_span(*reverted).subspan(0, reverted->size() - kPaddedHrpSize));

  return parts;
}

// https://zips.z.cash/zip-0316#encoding-of-unified-addresses
std::optional<std::string> ExtractTransparentPart(
    const std::string& unified_address,
    bool is_testnet) {
  auto transparent_bytes = GetTransparentRawBytes(unified_address, is_testnet);
  if (transparent_bytes) {
    return PubkeyHashToTransparentAddress(transparent_bytes.value(),
                                          is_testnet);
  }
  return std::nullopt;
}

std::optional<std::vector<uint8_t>> GetOrchardRawBytes(
    const std::string& unified_address,
    bool is_testnet) {
  auto parts = ExtractParsedAddresses(unified_address, is_testnet);

  if (!parts.has_value()) {
    return std::nullopt;
  }

  for (const auto& part : parts.value()) {
    if (part.first == ZCashAddrType::kOrchard) {
      return part.second;
    }
  }

  return std::nullopt;
}

std::optional<std::vector<uint8_t>> GetTransparentRawBytes(
    const std::string& unified_address,
    bool is_testnet) {
  auto parts = ExtractParsedAddresses(unified_address, is_testnet);
  if (!parts.has_value()) {
    return std::nullopt;
  }

  for (const auto& part : parts.value()) {
    if (part.first == ZCashAddrType::kP2PKH) {
      return part.second;
    }
  }

  return std::nullopt;
}

std::optional<std::string> ExtractOrchardPart(
    const std::string& unified_address,
    bool is_testnet) {
  auto bytes = GetOrchardRawBytes(unified_address, is_testnet);
  if (!bytes) {
    return std::nullopt;
  }
  return GetOrchardUnifiedAddress(bytes.value(), is_testnet);
}

std::optional<std::string> GetMergedUnifiedAddress(
    const std::vector<ParsedAddress>& parts,
    bool testnet) {
  if (parts.empty()) {
    return std::nullopt;
  }
  auto bytes = SerializeUnifiedAddress(parts);

  std::string hrp = testnet ? kTestnetHRP : kMainnetHRP;
  std::vector<uint8_t> padded_hrp(kPaddedHrpSize, 0);
  std::copy(reinterpret_cast<const uint8_t*>(hrp.c_str()),
            reinterpret_cast<const uint8_t*>(hrp.c_str()) + hrp.length(),
            padded_hrp.begin());
  bytes.insert(bytes.end(), padded_hrp.begin(), padded_hrp.end());

  auto jumbled = ApplyF4Jumble(bytes);
  if (!jumbled) {
    return std::nullopt;
  }

  std::vector<unsigned char> u5;
  ConvertBits<8, 5, true>([&](unsigned char c) { u5.push_back(c); },
                          jumbled.value().begin(), jumbled.value().end());
  auto encoded = bech32::Encode(bech32::Encoding::BECH32M, hrp, u5);
  if (encoded.empty()) {
    return std::nullopt;
  }
  return encoded;
}

std::optional<std::string> GetOrchardUnifiedAddress(
    base::span<const uint8_t> orchard_part,
    bool testnet) {
  return GetMergedUnifiedAddress(
      {ParsedAddress(
          ZCashAddrType::kOrchard,
          std::vector<uint8_t>(orchard_part.begin(), orchard_part.end()))},
      testnet);
}

}  // namespace brave_wallet
