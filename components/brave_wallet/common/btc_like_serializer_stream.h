/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BTC_LIKE_SERIALIZER_STREAM_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BTC_LIKE_SERIALIZER_STREAM_H_

#include <vector>

#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"

namespace brave_wallet {

class BtcLikeSerializerStream {
 public:
  BtcLikeSerializerStream();
  ~BtcLikeSerializerStream();

  void Push8(base::StrictNumeric<uint8_t> i);
  void Push16(base::StrictNumeric<uint16_t> i);
  void Push32(base::StrictNumeric<uint32_t> i);
  void Push64(base::StrictNumeric<uint64_t> i);
  void PushCompactSize(base::StrictNumeric<uint64_t> i);
  void PushSizeAndBytes(base::span<const uint8_t> bytes);
  void PushBytes(base::span<const uint8_t> bytes);
  void PushBytesReversed(base::span<const uint8_t> bytes);

  std::vector<uint8_t> Take() &&;

  const std::vector<uint8_t>& data() const { return data_; }

 private:
  std::vector<uint8_t> data_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BTC_LIKE_SERIALIZER_STREAM_H_
