/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"

#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"

namespace brave_wallet {

BtcLikeSerializerStream::BtcLikeSerializerStream() = default;
BtcLikeSerializerStream::~BtcLikeSerializerStream() = default;

void BtcLikeSerializerStream::Push8(base::StrictNumeric<uint8_t> i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push16(base::StrictNumeric<uint16_t> i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push32(base::StrictNumeric<uint32_t> i) {
  PushBytes(base::byte_span_from_ref(i));
}

void BtcLikeSerializerStream::Push64(base::StrictNumeric<uint64_t> i) {
  PushBytes(base::byte_span_from_ref(i));
}

// https://developer.bitcoin.org/reference/transactions.html#compactsize-unsigned-integers
void BtcLikeSerializerStream::PushCompactSize(base::StrictNumeric<uint64_t> i) {
  if (i < 0xfd) {
    Push8(base::checked_cast<uint8_t>(i));
  } else if (i <= 0xffff) {
    Push8(uint8_t{0xfd});
    Push16(base::checked_cast<uint16_t>(i));
  } else if (i <= 0xffffffff) {
    Push8(uint8_t{0xfe});
    Push32(base::checked_cast<uint32_t>(i));
  } else {
    Push8(uint8_t{0xff});
    Push64(i);
  }
}

void BtcLikeSerializerStream::PushSizeAndBytes(
    base::span<const uint8_t> bytes) {
  PushCompactSize(bytes.size());
  PushBytes(bytes);
}

void BtcLikeSerializerStream::PushBytes(base::span<const uint8_t> bytes) {
  data_.insert(data_.end(), bytes.begin(), bytes.end());
}

void BtcLikeSerializerStream::PushBytesReversed(
    base::span<const uint8_t> bytes) {
  data_.insert(data_.end(), bytes.rbegin(), bytes.rend());
}

std::vector<uint8_t> BtcLikeSerializerStream::Take() && {
  return std::move(data_);
}

}  // namespace brave_wallet
