/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_SERIALIZER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

class BitcoinSerializerStream {
 public:
  explicit BitcoinSerializerStream(std::vector<uint8_t>* to) : to_(to) {}

  void Push8AsLE(uint8_t i);
  void Push16AsLE(uint16_t i);
  void Push32AsLE(uint32_t i);
  void Push64AsLE(uint64_t i);
  void PushVarInt(uint64_t i);
  void PushSizeAndBytes(base::span<const uint8_t> bytes);
  void PushBytes(base::span<const uint8_t> bytes);
  void PushBytesReversed(base::span<const uint8_t> bytes);

  uint32_t serialized_bytes() const { return serialized_bytes_; }

 private:
  uint32_t serialized_bytes_ = 0;
  std::vector<uint8_t>* to() { return to_.get(); }
  raw_ptr<std::vector<uint8_t>> to_;
};

// TODO(apaymyshev): test with reference test vectors.
class BitcoinSerializer {
 public:
  static absl::optional<SHA256HashArray> SerializeInputForSign(
      const BitcoinTransaction& tx,
      size_t input_index);

  static std::vector<uint8_t> SerializeWitness(
      const std::vector<uint8_t>& signature,
      const std::vector<uint8_t>& pubkey);

  static std::vector<uint8_t> SerializeSignedTransaction(
      const BitcoinTransaction& tx);

  static uint32_t CalcTransactionWeight(const BitcoinTransaction& tx);
  static uint32_t CalcVSize(const BitcoinTransaction& tx);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_SERIALIZER_H_
