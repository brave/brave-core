
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/bitcoin_utils.h"

#include <utility>

#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/bech32.h"
#include "brave/third_party/bitcoin-core/src/src/util/strencodings.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

constexpr size_t kBech32WitnessMaxLength = 40;
constexpr size_t kBech32WitnessMinLength = 2;
constexpr size_t kP2WPKHLength = 20;
constexpr size_t kP2WSHLength = 32;

}  // namespace

namespace brave_wallet {

DecodedBitcoinAddress::DecodedBitcoinAddress() = default;
DecodedBitcoinAddress::~DecodedBitcoinAddress() = default;
DecodedBitcoinAddress::DecodedBitcoinAddress(
    const DecodedBitcoinAddress& other) = default;
DecodedBitcoinAddress& DecodedBitcoinAddress::operator=(
    const DecodedBitcoinAddress& other) = default;
DecodedBitcoinAddress::DecodedBitcoinAddress(DecodedBitcoinAddress&& other) =
    default;
DecodedBitcoinAddress& DecodedBitcoinAddress::operator=(
    DecodedBitcoinAddress&& other) = default;

absl::optional<DecodedBitcoinAddress> DecodeBitcoinAddress(
    const std::string& address) {
  // TODO(apaymyshev): support legacy addresses.
  auto bech_result = bech32::Decode(address);

  // https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#segwit-address-format
  if (bech_result.encoding != bech32::Encoding::BECH32) {
    return absl::nullopt;
  }

  DecodedBitcoinAddress result;
  if (bech_result.hrp == "tb") {
    result.testnet = true;
  } else if (bech_result.hrp == "bc") {
    result.testnet = false;
  } else {
    return absl::nullopt;
  }

  if (bech_result.data.empty() || bech_result.data[0] > 16) {
    return absl::nullopt;
  }

  std::vector<uint8_t> data;
  data.reserve(((bech_result.data.size() - 1) * 5) / 8);
  if (!ConvertBits<5, 8, false>([&](unsigned char c) { data.push_back(c); },
                                bech_result.data.begin() + 1,
                                bech_result.data.end())) {
    return absl::nullopt;
  }

  result.address_type = BitcoinAddressType::kWitnessUnknown;
  result.witness_version = bech_result.data[0];
  result.pubkey_hash = std::move(data);

  // https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki#witness-program
  if (result.witness_version == 0) {
    if (result.pubkey_hash.size() == kP2WPKHLength) {
      result.address_type = BitcoinAddressType::kWitnessV0PubkeyHash;
    } else if (result.pubkey_hash.size() == kP2WSHLength) {
      result.address_type = BitcoinAddressType::kWitnessV0ScriptHash;
    } else {
      return absl::nullopt;
    }
  }

  // https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#segwit-address-format
  if (result.pubkey_hash.size() < kBech32WitnessMinLength ||
      result.pubkey_hash.size() > kBech32WitnessMaxLength) {
    return absl::nullopt;
  }

  return result;
}

// https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#segwit-address-format
std::string PubkeyToSegwitAddress(const std::vector<uint8_t>& pubkey,
                                  bool testnet) {
  auto hash160 = Hash160(pubkey);
  std::vector<unsigned char> input;
  input.reserve(33);   // 1 + (160 / 5)
  input.push_back(0);  // the witness version
  ConvertBits<8, 5, true>([&](unsigned char c) { input.push_back(c); },
                          hash160.begin(), hash160.end());

  return bech32::Encode(bech32::Encoding::BECH32, testnet ? "tb" : "bc", input);
}

}  // namespace brave_wallet
