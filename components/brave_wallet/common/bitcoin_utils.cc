
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/bitcoin_utils.h"

#include <optional>
#include <utility>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/bech32.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace {

constexpr char kBech32MainnetHrp[] = "bc";
constexpr char kBech32TestnetHrp[] = "tb";
constexpr size_t kP2WPKHLength = 20;
constexpr size_t kP2WSHLength = 32;
constexpr size_t kP2TRLength = 32;
constexpr size_t kLegacyAddressLength = 21;  // 1 byte prefix + size(ripemd160)
// https://en.bitcoin.it/wiki/List_of_address_prefixes
constexpr uint8_t kP2PKHMainnetPrefix = 0;
constexpr uint8_t kP2PKHTestnetPrefix = 111;
constexpr uint8_t kP2SHMainnetPrefix = 5;
constexpr uint8_t kP2SHTestnetPrefix = 196;
constexpr uint8_t kSegwitWitnessVersion = 0;
constexpr uint8_t kTaprootWitnessVersion = 1;
}  // namespace

namespace brave_wallet {

namespace {

// https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#segwit-address-format
// https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki#addresses-for-segregated-witness-outputs
std::optional<DecodedBitcoinAddress> DecodeBech32Address(
    const std::string& address) {
  auto bech_result = bech32::DecodeForBitcoin(address);
  if (!bech_result) {
    return std::nullopt;
  }

  bool testnet = false;
  if (base::EqualsCaseInsensitiveASCII(bech_result->hrp, kBech32TestnetHrp)) {
    testnet = true;
  } else if (base::EqualsCaseInsensitiveASCII(bech_result->hrp,
                                              kBech32MainnetHrp)) {
    testnet = false;
  } else {
    return std::nullopt;
  }

  auto witness_version = bech_result->witness;

  // https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki#witness-program
  if (witness_version == kSegwitWitnessVersion &&
      bech_result->encoding == bech32::Encoding::kBech32) {
    if (bech_result->data.size() == kP2WPKHLength) {
      return DecodedBitcoinAddress(BitcoinAddressType::kWitnessV0PubkeyHash,
                                   std::move(bech_result->data), testnet);
    }
    if (bech_result->data.size() == kP2WSHLength) {
      return DecodedBitcoinAddress(BitcoinAddressType::kWitnessV0ScriptHash,
                                   std::move(bech_result->data), testnet);
    }
  } else if (witness_version == kTaprootWitnessVersion &&
             bech_result->encoding == bech32::Encoding::kBech32m) {
    if (bech_result->data.size() == kP2TRLength) {
      return DecodedBitcoinAddress(BitcoinAddressType::kWitnessV1Taproot,
                                   std::move(bech_result->data), testnet);
    }
  }

  return std::nullopt;
}

std::optional<DecodedBitcoinAddress> DecodeBase58Address(
    const std::string& address) {
  std::vector<uint8_t> decoded(kLegacyAddressLength);
  if (!DecodeBase58Check(address, decoded, kLegacyAddressLength)) {
    return std::nullopt;
  }

  if (decoded.size() != kLegacyAddressLength) {
    return std::nullopt;
  }

  std::vector<uint8_t> pubkey_hash(decoded.begin() + 1, decoded.end());
  DCHECK_EQ(pubkey_hash.size(), 20u);

  auto prefix = decoded[0];

  if (prefix == kP2PKHMainnetPrefix) {
    return DecodedBitcoinAddress(BitcoinAddressType::kPubkeyHash,
                                 std::move(pubkey_hash), false);
  } else if (prefix == kP2PKHTestnetPrefix) {
    return DecodedBitcoinAddress(BitcoinAddressType::kPubkeyHash,
                                 std::move(pubkey_hash), true);
  } else if (prefix == kP2SHMainnetPrefix) {
    return DecodedBitcoinAddress(BitcoinAddressType::kScriptHash,
                                 std::move(pubkey_hash), false);
  } else if (prefix == kP2SHTestnetPrefix) {
    return DecodedBitcoinAddress(BitcoinAddressType::kScriptHash,
                                 std::move(pubkey_hash), true);
  } else {
    return std::nullopt;
  }
}

}  // namespace

DecodedBitcoinAddress::DecodedBitcoinAddress() = default;
DecodedBitcoinAddress::DecodedBitcoinAddress(BitcoinAddressType address_type,
                                             std::vector<uint8_t> pubkey_hash,
                                             bool testnet)
    : address_type(address_type),
      pubkey_hash(std::move(pubkey_hash)),
      testnet(testnet) {}
DecodedBitcoinAddress::~DecodedBitcoinAddress() = default;
DecodedBitcoinAddress::DecodedBitcoinAddress(
    const DecodedBitcoinAddress& other) = default;
DecodedBitcoinAddress& DecodedBitcoinAddress::operator=(
    const DecodedBitcoinAddress& other) = default;
DecodedBitcoinAddress::DecodedBitcoinAddress(DecodedBitcoinAddress&& other) =
    default;
DecodedBitcoinAddress& DecodedBitcoinAddress::operator=(
    DecodedBitcoinAddress&& other) = default;

std::optional<DecodedBitcoinAddress> DecodeBitcoinAddress(
    const std::string& address) {
  if (base::StartsWith(address, kBech32MainnetHrp,
                       base::CompareCase::INSENSITIVE_ASCII) ||
      base::StartsWith(address, kBech32TestnetHrp,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return DecodeBech32Address(address);
  }

  return DecodeBase58Address(address);
}

// https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#segwit-address-format
std::string PubkeyToSegwitAddress(base::span<const uint8_t> pubkey,
                                  bool testnet) {
  return bech32::EncodeForBitcoin(
      Hash160(pubkey), testnet ? kBech32TestnetHrp : kBech32MainnetHrp,
      kSegwitWitnessVersion);
}

uint64_t ApplyFeeRate(double fee_rate, uint32_t vbytes) {
  // Bitcoin core does ceiling here.
  // https://github.com/bitcoin/bitcoin/blob/v25.1/src/policy/feerate.cpp#L29
  return static_cast<uint64_t>(std::ceil(fee_rate * vbytes));
}

}  // namespace brave_wallet
