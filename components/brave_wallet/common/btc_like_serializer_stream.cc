/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"

#include "base/containers/span.h"

namespace brave_wallet {

void BtcLikeSerializerStream::Push8(uint8_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push16(uint16_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push32(uint32_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push64(uint64_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

// https://developer.bitcoin.org/reference/transactions.html#compactsize-unsigned-integers
void BtcLikeSerializerStream::PushCompactSize(uint64_t i) {
  if (i < 0xfd) {
    Push8(i);
  } else if (i <= 0xffff) {
    Push8(0xfd);
    Push16(i);
  } else if (i <= 0xffffffff) {
    Push8(0xfe);
    Push32(i);
  } else {
    Push8(0xff);
    Push64(i);
  }
}

void BtcLikeSerializerStream::PushSizeAndBytes(
    base::span<const uint8_t> bytes) {
  PushCompactSize(bytes.size());
  PushBytes(bytes);
}

void BtcLikeSerializerStream::PushBytes(base::span<const uint8_t> bytes) {
  if (to()) {
    to()->insert(to()->end(), bytes.begin(), bytes.end());
  }
  serialized_bytes_ += bytes.size();
}

void BtcLikeSerializerStream::PushBytesReversed(
    base::span<const uint8_t> bytes) {
  if (to()) {
    to()->insert(to()->end(), bytes.rbegin(), bytes.rend());
  }
  serialized_bytes_ += bytes.size();
}

}  // namespace brave_wallet
