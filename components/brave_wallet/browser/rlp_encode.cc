/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/rlp_encode.h"

#include <algorithm>

#include "base/containers/extend.h"
#include "base/containers/span.h"
#include "base/numerics/checked_math.h"

namespace {

constexpr uint8_t kStringOffset = 0x80;
constexpr uint8_t kListOffset = 0xc0;
constexpr uint8_t kSingleByteLengthLimit = 55;

std::vector<uint8_t> RLPToBinary(size_t x) {
  if (x == 0) {
    return {};
  }
  std::vector<uint8_t> ret = RLPToBinary(x / 256);
  ret.push_back(x % 256);
  return ret;
}

std::vector<uint8_t> RLPEncodeLength(size_t length, uint8_t offset) {
  if (length <= kSingleByteLengthLimit) {
    return std::vector<uint8_t>(1, length + offset);
  }
  std::vector<uint8_t> length_encoded = RLPToBinary(length);
  std::vector<uint8_t> result(length_encoded.size() + 1);
  result[0] =
      base::CheckAdd(length_encoded.size(), offset, kSingleByteLengthLimit)
          .ValueOrDie<uint8_t>();
  base::span(result).subspan(1).copy_from(length_encoded);
  return result;
}

}  // namespace

namespace brave_wallet {

base::Value::BlobStorage RLPUint256ToBlob(uint256_t input) {
  base::Value::BlobStorage output;
  while (input > 0) {
    output.push_back(static_cast<uint8_t>(input & 0xFF));
    input >>= 8;
  }
  // Return the vector reversed (big endian)
  std::reverse(output.begin(), output.end());
  return output;
}

std::vector<uint8_t> RLPEncode(const base::Value& val) {
  if (val.is_int()) {
    return RLPEncode(base::Value(RLPUint256ToBlob(val.GetInt())));
  }

  if (val.is_blob()) {
    base::Value::BlobStorage blob = val.GetBlob();
    if (blob.size() == 1 && blob[0] < kStringOffset) {
      return blob;
    }
    auto result = RLPEncodeLength(blob.size(), kStringOffset);
    base::Extend(result, blob);
    return result;
  }

  if (val.is_string()) {
    return RLPEncode(base::Value(base::as_byte_span(val.GetString())));
  }

  if (val.is_list()) {
    return RLPEncode(val.GetList());
  }

  return {};
}

std::vector<uint8_t> RLPEncode(const base::Value::List& val) {
  std::vector<uint8_t> items_encoded;
  for (auto& item : val) {
    base::Extend(items_encoded, RLPEncode(item));
  }
  auto result = RLPEncodeLength(items_encoded.size(), kListOffset);
  base::Extend(result, items_encoded);
  return result;
}

}  // namespace brave_wallet
