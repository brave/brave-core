/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BTC_LIKE_SERIALIZER_STREAM_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BTC_LIKE_SERIALIZER_STREAM_H_

#include <vector>

#include "base/containers/span.h"
#include "base/memory/raw_ptr.h"

namespace brave_wallet {

class BtcLikeSerializerStream {
 public:
  explicit BtcLikeSerializerStream(std::vector<uint8_t>* to) : to_(to) {}

  void Push8(uint8_t i);
  void Push16(uint16_t i);
  void Push32(uint32_t i);
  void Push64(uint64_t i);
  void PushCompactSize(uint64_t i);
  void PushSizeAndBytes(base::span<const uint8_t> bytes);
  void PushBytes(base::span<const uint8_t> bytes);
  void PushBytesReversed(base::span<const uint8_t> bytes);

  uint32_t serialized_bytes() const { return serialized_bytes_; }

 private:
  uint32_t serialized_bytes_ = 0;
  std::vector<uint8_t>* to() { return to_.get(); }
  raw_ptr<std::vector<uint8_t>> to_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BTC_LIKE_SERIALIZER_STREAM_H_
