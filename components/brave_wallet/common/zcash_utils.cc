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
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
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

std::array<uint8_t, kPaddedHrpSize> GetPaddedHRP(bool is_testnet) {
  static_assert(kPaddedHrpSize > sizeof(kTestnetHRP) &&
                    kPaddedHrpSize > sizeof(kMainnetHRP),
                "Wrong kPaddedHrpSize size");
  std::string hrp = is_testnet ? kTestnetHRP : kMainnetHRP;
  std::array<uint8_t, kPaddedHrpSize> padded_hrp = {};
  base::ranges::copy(base::make_span(hrp), padded_hrp.begin());
  return padded_hrp;
}

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

std::vector<uint8_t> SerializeUnifiedAddress(
    const std::vector<ParsedAddress>& parts) {
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

base::Value::Dict OrchardOutput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("address", base::HexEncode(addr.data(), addr.size()));
  dict.Set("amount", base::NumberToString(value));
  if (memo) {
    dict.Set("memo", base::HexEncode(memo.value()));
  }

  return dict;
}

// static
std::optional<OrchardOutput> OrchardOutput::FromValue(
    const base::Value::Dict& value) {
  OrchardOutput result;
  if (!ReadHexByteArrayTo<kOrchardRawBytesSize>(value, "address",
                                                result.addr)) {
    return std::nullopt;
  }

  if (!ReadUint32StringTo(value, "amount", result.value)) {
    return std::nullopt;
  }

  if (value.contains("memo")) {
    auto memo = OrchardMemo();
    if (!ReadHexByteArrayTo<kOrchardMemoSize>(value, "memo", memo)) {
      return std::nullopt;
    }
    result.memo = memo;
  }

  return result;
}

bool OutputZCashAddressSupported(const std::string& address, bool is_testnet) {
  auto decoded_address = DecodeZCashAddress(address);
  if (!decoded_address) {
    return false;
  }
  if (decoded_address->testnet != is_testnet) {
    return false;
  }

  return true;
}

// https://zips.z.cash/zip-0317
uint64_t CalculateZCashTxFee(const uint32_t tx_input_count,
                             const uint32_t orchard_actions_count) {
  // Use simplified calcultion fee form since we don't support p2psh
  // and shielded addresses
  auto actions_count = std::max(tx_input_count + orchard_actions_count,
                                kDefaultTransparentOutputsCount);
  return kMarginalFee * std::max(kGraceActionsCount, actions_count);
}

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
                                                bool is_testnet) {
  auto decoded_address = DecodeZCashAddress(address);
  if (!decoded_address) {
    return {};
  }

  if (is_testnet != decoded_address->testnet) {
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

  auto padded_hrp = GetPaddedHRP(is_testnet);

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

std::optional<OrchardAddrRawPart> GetOrchardRawBytes(
    const std::string& unified_address,
    bool is_testnet) {
  auto parts = ExtractParsedAddresses(unified_address, is_testnet);

  if (!parts.has_value()) {
    return std::nullopt;
  }

  for (const auto& part : parts.value()) {
    if (part.first == ZCashAddrType::kOrchard) {
      if (part.second.size() != kOrchardRawBytesSize) {
        return std::nullopt;
      }
      OrchardAddrRawPart result;
      base::ranges::copy(part.second, result.begin());
      return result;
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

std::optional<std::string> RevertHex(const std::string& hex) {
  if (hex.empty()) {
    return std::nullopt;
  }
  std::vector<uint8_t> bytes;
  if (!PrefixedHexStringToBytes("0x" + hex, &bytes)) {
    return std::nullopt;
  }
  std::reverse(bytes.begin(), bytes.end());
  return ToHex(bytes);
}

std::optional<std::string> GetMergedUnifiedAddress(
    const std::vector<ParsedAddress>& parts,
    bool is_testnet) {
  if (parts.empty()) {
    return std::nullopt;
  }
  auto bytes = SerializeUnifiedAddress(parts);

  auto padded_hrp = GetPaddedHRP(is_testnet);
  bytes.insert(bytes.end(), padded_hrp.begin(), padded_hrp.end());

  auto jumbled = ApplyF4Jumble(bytes);
  if (!jumbled) {
    return std::nullopt;
  }

  std::vector<unsigned char> u5;
  ConvertBits<8, 5, true>([&](unsigned char c) { u5.push_back(c); },
                          jumbled.value().begin(), jumbled.value().end());
  auto encoded = bech32::Encode(bech32::Encoding::BECH32M,
                                is_testnet ? kTestnetHRP : kMainnetHRP, u5);
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

std::optional<OrchardMemo> ToOrchardMemo(
    const std::optional<std::vector<uint8_t>>& input) {
  if (!input) {
    return std::nullopt;
  }

  if (input->size() > kOrchardMemoSize) {
    return std::nullopt;
  }

  std::array<uint8_t, kOrchardMemoSize> output = {};
  base::ranges::copy(*input, output.begin());
  return output;
}

std::optional<std::vector<uint8_t>> OrchardMemoToVec(
    const std::optional<OrchardMemo>& memo) {
  if (!memo) {
    return std::nullopt;
  }

  return std::vector<uint8_t>{memo->begin(), memo->end()};
}

}  // namespace brave_wallet
