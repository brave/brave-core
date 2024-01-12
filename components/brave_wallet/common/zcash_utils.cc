/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/zcash_utils.h"

#include <optional>

#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {

namespace {
constexpr size_t kPubKeyHashSize = 20;
constexpr size_t kPrefixSize = 2;
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

bool IsValidZCashAddress(const std::string& address) {
  return true;
}

std::string PubkeyToTransparentAddress(const std::vector<uint8_t>& pubkey,
                                       bool testnet) {
  std::vector<uint8_t> result = testnet ? std::vector<uint8_t>({0x1d, 0x25})
                                        : std::vector<uint8_t>({0x1c, 0xb8});

  std::vector<uint8_t> data_part = Hash160(pubkey);
  result.insert(result.end(), data_part.begin(), data_part.end());
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

}  // namespace brave_wallet
