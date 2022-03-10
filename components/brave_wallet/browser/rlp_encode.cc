/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/rlp_encode.h"

#include <algorithm>
#include <utility>

namespace {

std::string RLPToBinary(size_t x) {
  if (x == 0) {
    return "";
  }
  std::string ret = RLPToBinary(x / 256);
  ret.push_back(x % 256);
  return ret;
}

std::string RLPEncodeLength(size_t length, size_t offset) {
  char sz[2] = {0};
  if (length < 56) {
    sz[0] = length + offset;
    return sz;
  }
  std::string BL = RLPToBinary(length);
  sz[0] = BL.length() + offset + 55;
  std::string ret(sz);
  ret.append(BL);
  return ret;
}

}  // namespace

namespace brave_wallet {

base::Value RLPUint256ToBlobValue(uint256_t input) {
  base::Value::BlobStorage output;
  while (input > static_cast<uint256_t>(0)) {
    uint8_t i = static_cast<uint8_t>(input & static_cast<uint256_t>(0xFF));
    output.push_back(static_cast<char>(i));
    input >>= 8;
  }
  // Return the vector reversed (big endian)
  std::reverse(output.begin(), output.end());
  return base::Value(output);
}

std::string RLPEncode(base::Value val) {
  if (val.is_int()) {
    int i = val.GetInt();
    return RLPEncode(base::Value(RLPUint256ToBlobValue((uint256_t)i)));
  } else if (val.is_blob()) {
    base::Value::BlobStorage blob = val.GetBlob();
    std::string s(blob.begin(), blob.end());
    if (blob.size() == 1 && static_cast<uint8_t>(blob[0]) < 0x80) {
      return s;
    }
    return RLPEncodeLength(blob.size(), 0x80) + s;
  } else if (val.is_string()) {
    std::string s = val.GetString();
    return RLPEncode(base::Value(base::Value::BlobStorage(s.begin(), s.end())));
  } else if (val.is_list()) {
    std::string output;
    for (auto& item : val.GetList()) {
      output += RLPEncode(std::move(item));
    }
    return RLPEncodeLength(output.length(), 0xc0) + output;
  }
  return "";
}

}  // namespace brave_wallet
