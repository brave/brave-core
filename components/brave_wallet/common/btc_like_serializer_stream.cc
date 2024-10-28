/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"

#include "base/containers/span.h"

namespace brave_wallet {

void BtcLikeSerializerStream::Push8AsLE(uint8_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push16AsLE(uint16_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push32AsLE(uint32_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push64AsLE(uint64_t i) {
  PushBytes(base::byte_span_from_ref(i));
}

// https://developer.bitcoin.org/reference/transactions.html#compactsize-unsigned-integers
void BtcLikeSerializerStream::PushVarInt(uint64_t i) {
  if (i < 0xfd) {
    Push8AsLE(i);
  } else if (i <= 0xffff) {
    Push8AsLE(0xfd);
    Push16AsLE(i);
  } else if (i <= 0xffffffff) {
    Push8AsLE(0xfe);
    Push32AsLE(i);
  } else {
    Push8AsLE(0xff);
    Push64AsLE(i);
  }
}

void BtcLikeSerializerStream::PushSizeAndBytes(
    base::span<const uint8_t> bytes) {
  PushVarInt(bytes.size());
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

void BtcLikeSerializerStream::PushCompactSize(uint64_t i) {
  if (i < 253) {
    Push8AsLE(static_cast<uint8_t>(i));
  } else if (i < 0xFFFF) {
    Push8AsLE(253);
    Push16AsLE(static_cast<uint16_t>(i));

  } else if (i < 0xFFFFFFFF) {
    Push8AsLE(254);
    Push32AsLE(static_cast<uint32_t>(i));
  } else {
    Push8AsLE(255);
    Push32AsLE(static_cast<uint64_t>(i));
  }
}

}  // namespace brave_wallet
