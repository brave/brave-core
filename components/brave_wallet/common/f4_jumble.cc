/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/f4_jumble.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "base/containers/span.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"

namespace brave_wallet {

// Sizes for Blake2b are defined here https://zips.z.cash/zip-0316#solution
const size_t kMinMessageSize = 48u;
const size_t kMaxMessageSize = 4184368u;
const size_t kLeftSize = 64u;

std::array<uint8_t, 16> GetHPersonalizer(uint8_t i) {
  return {85,  65, 95,  70,  52, 74, 117,
          109, 98, 108, 101, 95, 72, static_cast<uint8_t>(i),
          0,   0};
}

std::array<uint8_t, 16> GetGPersonalizer(uint8_t i, uint16_t j) {
  return {85,
          65,
          95,
          70,
          52,
          74,
          117,
          109,
          98,
          108,
          101,
          95,
          71,
          static_cast<uint8_t>(i),
          static_cast<uint8_t>(j & 0xFF),
          static_cast<uint8_t>(j >> 8)};
}

std::vector<uint8_t> blake2b(const std::vector<uint8_t>& payload,
                             const std::array<uint8_t, 16>& personalizer,
                             size_t len) {
  blake2b_state blake_state = {};
  blake2b_param params = {};

  params.digest_length = len;
  params.fanout = 1;
  params.depth = 1;
  if (personalizer.size() != sizeof(params.personal)) {
    NOTREACHED_NORETURN();
  }
  memcpy(params.personal, personalizer.data(), sizeof(params.personal));
  if (blake2b_init_param(&blake_state, &params) != 0) {
    NOTREACHED_NORETURN();
  }
  if (blake2b_update(&blake_state, payload.data(), payload.size()) != 0) {
    NOTREACHED_NORETURN();
  }
  std::vector<uint8_t> result(len);
  if (blake2b_final(&blake_state, result.data(), len) != 0) {
    NOTREACHED_NORETURN();
  }

  return result;
}

std::vector<uint8_t> GetLeft(const std::vector<uint8_t>& message) {
  size_t left_size = std::min(kLeftSize, message.size() / 2);
  return std::vector(message.begin(), message.begin() + left_size);
}

std::vector<uint8_t> GetRight(const std::vector<uint8_t>& message) {
  size_t left_size = std::min(kLeftSize, message.size() / 2);
  return std::vector(message.begin() + left_size, message.end());
}

std::vector<uint8_t> h_round(uint8_t iter,
                             const std::vector<uint8_t>& left,
                             const std::vector<uint8_t>& right) {
  auto hash = blake2b(right, GetHPersonalizer(iter), left.size());
  if (hash.size() != left.size()) {
    NOTREACHED_NORETURN();
  }
  std::vector<uint8_t> result;
  for (size_t i = 0; i < left.size(); i++) {
    result.push_back(left[i] ^ hash[i]);
  }
  return result;
}

std::vector<uint8_t> g_round(uint8_t i,
                             const std::vector<uint8_t>& left,
                             const std::vector<uint8_t>& right) {
  size_t blocks_count = (right.size() + kLeftSize - 1) / kLeftSize;
  std::vector<uint8_t> result;
  for (size_t j = 0; j < blocks_count; j++) {
    auto hash = blake2b(left, GetGPersonalizer(i, j), kLeftSize);
    if (hash.size() != kLeftSize) {
      NOTREACHED_NORETURN();
    }
    for (size_t k = 0; k < hash.size() && (j * kLeftSize + k < right.size());
         k++) {
      result.push_back(right[j * kLeftSize + k] ^ hash[k]);
    }
  }
  return result;
}

std::optional<std::vector<uint8_t>> ApplyF4Jumble(
    const std::vector<uint8_t>& message) {
  if (message.size() < kMinMessageSize || message.size() > kMaxMessageSize) {
    return std::nullopt;
  }

  auto left = GetLeft(message);
  auto right = GetRight(message);

  right = g_round(0, left, right);
  left = h_round(0, left, right);
  right = g_round(1, left, right);
  left = h_round(1, left, right);

  left.insert(left.end(), right.begin(), right.end());
  return left;
}

std::optional<std::vector<uint8_t>> RevertF4Jumble(
    const std::vector<uint8_t>& jumbled_message) {
  if (jumbled_message.size() < kMinMessageSize ||
      jumbled_message.size() > kMaxMessageSize) {
    return std::nullopt;
  }

  auto left = GetLeft(jumbled_message);
  auto right = GetRight(jumbled_message);

  left = h_round(1, left, right);
  right = g_round(1, left, right);
  left = h_round(0, left, right);
  right = g_round(0, left, right);

  left.insert(left.end(), right.begin(), right.end());
  return left;
}

}  // namespace brave_wallet
