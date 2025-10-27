/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/zcash_utils.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/containers/extend.h"
#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/bech32.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/f4_jumble.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {

namespace {
constexpr char kTestnetHRP[] = "utest";
constexpr char kMainnetHRP[] = "u";
constexpr size_t kPaddedHrpSize = 16;
constexpr size_t kPubKeyHashSize = 20;
constexpr size_t kPrefixSize = 2;

std::array<uint8_t, kPaddedHrpSize> GetPaddedHRP(std::string_view hrp) {
  CHECK_LE(hrp.size(), kPaddedHrpSize);
  std::array<uint8_t, kPaddedHrpSize> padded_hrp = {};
  std::ranges::copy(base::as_byte_span(hrp), padded_hrp.begin());
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
    data = data.subspan(1u);
  } else if (type == 253 && data.size() >= 3) {
    value = base::U16FromBigEndian(data.subspan<1, 2u>());
    data = data.subspan(1u + 2);
  } else if (type <= 254 && data.size() >= 5) {
    value = base::U32FromBigEndian(data.subspan<1, 4u>());
    data = data.subspan(1u + 4);
  } else if (data.size() >= 9) {
    value = base::U64FromBigEndian(data.subspan<1, 8u>());
    data = data.subspan(1u + 8);
  } else {
    return std::nullopt;
  }
  return value;
}

std::vector<uint8_t> SerializeUnifiedAddress(
    const std::vector<ParsedAddress>& parts) {
  BtcLikeSerializerStream stream;
  for (const auto& part : parts) {
    stream.PushCompactSize(part.first);
    stream.PushSizeAndBytes(part.second);
  }
  return std::move(stream).Take();
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
    dejumbled_data = dejumbled_data.subspan(base::checked_cast<size_t>(*size));
  }
  return result;
}

std::vector<uint8_t> GetNetworkPrefix(bool is_testnet) {
  return is_testnet ? std::vector<uint8_t>({0x1d, 0x25})
                    : std::vector<uint8_t>({0x1c, 0xb8});
}

}  // namespace

base::Value::Dict OrchardNote::ToValue() const {
  base::Value::Dict dict;

  dict.Set("addr", base::HexEncode(addr));
  dict.Set("block_id", base::NumberToString(block_id));
  dict.Set("nullifier", base::HexEncode(nullifier));
  dict.Set("amount", base::NumberToString(amount));
  dict.Set("orchard_commitment_tree_position",
           base::NumberToString(orchard_commitment_tree_position));
  dict.Set("rho", base::HexEncode(rho));
  dict.Set("seed", base::HexEncode(seed));

  return dict;
}

base::expected<void, mojom::ZCashAddressError>
ValidateTransparentRecipientAddress(bool testnet, const std::string& addr) {
  if (IsUnifiedAddress(addr)) {
    if (IsUnifiedTestnetAddress(addr) != testnet) {
      return base::unexpected(
          mojom::ZCashAddressError::kInvalidAddressNetworkMismatch);
    }
    if (!ExtractTransparentPart(addr, testnet)) {
      return base::unexpected(mojom::ZCashAddressError::
                                  kInvalidUnifiedAddressMissingTransparentPart);
    }
    return base::ok();
  }

  auto decoded = DecodeZCashTransparentAddress(addr);
  if (!decoded) {
    return base::unexpected(
        mojom::ZCashAddressError::kInvalidTransparentAddress);
  }

  if (decoded->testnet != testnet) {
    return base::unexpected(
        mojom::ZCashAddressError::kInvalidAddressNetworkMismatch);
  }

  return base::ok();
}

base::expected<void, mojom::ZCashAddressError> ValidateOrchardRecipientAddress(
    bool testnet,
    const std::string& addr) {
  if (!IsUnifiedAddress(addr)) {
    return base::unexpected(mojom::ZCashAddressError::kInvalidUnifiedAddress);
  }

  if (IsUnifiedTestnetAddress(addr) != testnet) {
    return base::unexpected(
        mojom::ZCashAddressError::kInvalidAddressNetworkMismatch);
  }

  if (!ExtractOrchardPart(addr, testnet)) {
    return base::unexpected(
        mojom::ZCashAddressError::kInvalidUnifiedAddressMissingOrchardPart);
  }

  return base::ok();
}

// static
std::optional<OrchardNote> OrchardNote::FromValue(
    const base::Value::Dict& value) {
  OrchardNote result;
  if (!ReadHexByteArrayTo<kOrchardRawBytesSize>(value, "addr", result.addr)) {
    return std::nullopt;
  }

  if (!ReadUint32StringTo(value, "block_id", result.block_id)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo<kOrchardNullifierSize>(value, "nullifier",
                                                 result.nullifier)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount)) {
    return std::nullopt;
  }

  if (!ReadUint32StringTo(value, "orchard_commitment_tree_position",
                          result.orchard_commitment_tree_position)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo<kOrchardNoteRhoSize>(value, "rho", result.rho)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo<kOrchardNoteRSeedSize>(value, "seed", result.seed)) {
    return std::nullopt;
  }

  return result;
}

base::Value::Dict OrchardInput::ToValue() const {
  base::Value::Dict dict;

  // Do not serialize witness ATM since it is calculated before post
  dict.Set("note", note.ToValue());

  return dict;
}

// static
std::optional<OrchardInput> OrchardInput::FromValue(
    const base::Value::Dict& value) {
  OrchardInput result;

  auto* note_dict = value.FindDict("note");
  if (!note_dict) {
    return std::nullopt;
  }
  auto note = OrchardNote::FromValue(*note_dict);
  if (!note) {
    return std::nullopt;
  }

  result.note = *note;

  return result;
}

OrchardNoteWitness::OrchardNoteWitness() = default;
OrchardNoteWitness::~OrchardNoteWitness() = default;
OrchardNoteWitness::OrchardNoteWitness(const OrchardNoteWitness& other) =
    default;

OrchardInput::OrchardInput() = default;
OrchardInput::~OrchardInput() = default;
OrchardInput::OrchardInput(const OrchardInput& other) = default;

OrchardSpendsBundle::OrchardSpendsBundle() = default;
OrchardSpendsBundle::~OrchardSpendsBundle() = default;
OrchardSpendsBundle::OrchardSpendsBundle(const OrchardSpendsBundle& other) =
    default;

DecodedZCashTransparentAddress::DecodedZCashTransparentAddress() = default;
DecodedZCashTransparentAddress::~DecodedZCashTransparentAddress() = default;
DecodedZCashTransparentAddress::DecodedZCashTransparentAddress(
    const DecodedZCashTransparentAddress& other) = default;
DecodedZCashTransparentAddress& DecodedZCashTransparentAddress::operator=(
    const DecodedZCashTransparentAddress& other) = default;
DecodedZCashTransparentAddress::DecodedZCashTransparentAddress(
    DecodedZCashTransparentAddress&& other) = default;
DecodedZCashTransparentAddress& DecodedZCashTransparentAddress::operator=(
    DecodedZCashTransparentAddress&& other) = default;

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

  if (!ReadUint64StringTo(value, "amount", result.value)) {
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

bool OutputZCashTransparentAddressSupported(const std::string& address,
                                            bool is_testnet) {
  auto decoded_address = DecodeZCashTransparentAddress(address);
  if (!decoded_address) {
    return false;
  }
  if (decoded_address->testnet != is_testnet) {
    return false;
  }

  return true;
}

// https://zips.z.cash/zip-0317
base::CheckedNumeric<uint64_t> CalculateZCashTxFee(
    const uint32_t tx_input_count,
    const uint32_t orchard_actions_count) {
  // Use simplified calculation fee form since we don't support p2psh
  // and shielded addresses
  auto actions_count = base::CheckMax(
      base::CheckAdd<uint32_t>(tx_input_count, orchard_actions_count),
      kDefaultTransparentOutputsCount);
  return base::CheckMul<uint64_t>(
      kMarginalFee, base::CheckMax(kGraceActionsCount, actions_count));
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

  base::Extend(result, Hash160(pubkey));

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

std::optional<DecodedZCashTransparentAddress> DecodeZCashTransparentAddress(
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

  DecodedZCashTransparentAddress result;
  result.pubkey_hash = body;
  result.testnet = is_testnet;

  return result;
}

std::vector<uint8_t> ZCashAddressToScriptPubkey(const std::string& address,
                                                bool is_testnet) {
  auto decoded_address = DecodeZCashTransparentAddress(address);
  if (!decoded_address) {
    return {};
  }

  if (is_testnet != decoded_address->testnet) {
    return {};
  }

  BtcLikeSerializerStream stream;
  CHECK_EQ(decoded_address->pubkey_hash.size(), 20u);

  stream.Push8(uint8_t{0x76u});                    // OP_DUP
  stream.Push8(uint8_t{0xa9u});                    // OP_HASH
  stream.Push8(uint8_t{0x14u});                    // hash size
  stream.PushBytes(decoded_address->pubkey_hash);  // hash
  stream.Push8(uint8_t{0x88u});                    // OP_EQUALVERIFY
  stream.Push8(uint8_t{0xacu});                    // OP_CHECKSIG

  return std::move(stream).Take();
}

std::optional<std::vector<ParsedAddress>> ExtractParsedAddresses(
    const std::string& unified_address,
    bool is_testnet) {
  auto bech_result = bech32::Decode(unified_address);
  if (!bech_result) {
    return std::nullopt;
  }

  if (bech_result->encoding != bech32::Encoding::kBech32m) {
    return std::nullopt;
  }

  std::string expected_hrp = is_testnet ? kTestnetHRP : kMainnetHRP;
  if (bech_result->hrp != expected_hrp) {
    return std::nullopt;
  }

  auto reverted = RevertF4Jumble(bech_result->data);
  // HRP with 16 bytes padding which is appended to the end of message
  if (!reverted || reverted->size() < kPaddedHrpSize) {
    return std::nullopt;
  }

  auto [body, padded_hrp] =
      base::span(*reverted).split_at(reverted->size() - kPaddedHrpSize);

  // Check that HRP equals padded HRP
  if (GetPaddedHRP(expected_hrp) != padded_hrp) {
    return std::nullopt;
  }

  return ParseUnifiedAddressBody(body);
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
      std::ranges::copy(part.second, result.begin());
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
  std::string_view hrp = is_testnet ? kTestnetHRP : kMainnetHRP;

  auto bytes = SerializeUnifiedAddress(parts);
  base::Extend(bytes, GetPaddedHRP(hrp));

  auto jumbled = ApplyF4Jumble(bytes);
  if (!jumbled) {
    return std::nullopt;
  }

  auto encoded = bech32::Encode(*jumbled, hrp, bech32::Encoding::kBech32m);
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
  std::ranges::copy(*input, output.begin());
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
