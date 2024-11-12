/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/f4_jumble.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/notreached.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"

namespace brave_wallet {

namespace {

using Blake2bPersonalBytes = std::array<uint8_t, BLAKE2B_PERSONALBYTES>;

// Sizes for Blake2b are defined here https://zips.z.cash/zip-0316#solution
constexpr size_t kMinMessageSize = 48u;
constexpr size_t kMaxMessageSize = 4184368u;
constexpr size_t kLeftSize = 64u;
static_assert(kLeftSize <= BLAKE2B_OUTBYTES);

Blake2bPersonalBytes GetHPersonalizer(uint8_t i) {
  return {85, 65, 95, 70, 52, 74, 117, 109, 98, 108, 101, 95, 72, i, 0, 0};
}

Blake2bPersonalBytes GetGPersonalizer(uint8_t i, uint16_t j) {
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
          i,
          static_cast<uint8_t>(j & 0xFF),
          static_cast<uint8_t>(j >> 8)};
}

void FillBlake2bParamPersonal(const Blake2bPersonalBytes& personalizer,
                              blake2b_param& params) {
  CHECK_EQ(personalizer.size(), sizeof(params.personal));
  memcpy(params.personal, personalizer.data(), sizeof(params.personal));
}

std::vector<uint8_t> Blake2b(base::span<const uint8_t> payload,
                             const Blake2bPersonalBytes& personalizer,
                             size_t digest_len) {
  CHECK(digest_len >= 1 && digest_len <= BLAKE2B_OUTBYTES);
  blake2b_state blake_state = {};
  blake2b_param params = {};

  params.digest_length = digest_len;
  params.fanout = 1;
  params.depth = 1;

  FillBlake2bParamPersonal(personalizer, params);

  CHECK_EQ(blake2b_init_param(&blake_state, &params), 0);
  CHECK_EQ(blake2b_update(&blake_state, payload.data(), payload.size()), 0);
  std::vector<uint8_t> result(digest_len);
  CHECK_EQ(blake2b_final(&blake_state, result.data(), digest_len), 0);
  return result;
}

size_t GetLeftSize(base::span<const uint8_t> message) {
  return std::min(kLeftSize, message.size() / 2);
}

std::vector<uint8_t> GetLeft(base::span<const uint8_t> message) {
  return std::vector(message.begin(), message.begin() + GetLeftSize(message));
}

std::vector<uint8_t> GetRight(base::span<const uint8_t> message) {
  return std::vector(message.begin() + GetLeftSize(message), message.end());
}

std::vector<uint8_t> HRound(uint8_t iter,
                            base::span<const uint8_t> left,
                            base::span<const uint8_t> right) {
  auto hash = Blake2b(right, GetHPersonalizer(iter), left.size());
  CHECK_EQ(hash.size(), left.size());
  std::vector<uint8_t> result;
  result.reserve(left.size());
  for (size_t i = 0; i < left.size(); i++) {
    result.push_back(left[i] ^ hash[i]);
  }
  return result;
}

std::vector<uint8_t> GRound(uint8_t i,
                            base::span<const uint8_t> left,
                            base::span<const uint8_t> right) {
  size_t blocks_count = (right.size() + kLeftSize - 1) / kLeftSize;
  std::vector<uint8_t> result;
  result.reserve(right.size());
  for (size_t j = 0; j < blocks_count; j++) {
    auto hash = Blake2b(left, GetGPersonalizer(i, j), kLeftSize);
    CHECK_EQ(hash.size(), kLeftSize);
    for (size_t k = 0; k < hash.size() && (j * kLeftSize + k < right.size());
         k++) {
      result.push_back(right[j * kLeftSize + k] ^ hash[k]);
    }
  }
  return result;
}

}  // namespace

std::optional<std::vector<uint8_t>> ApplyF4Jumble(
    base::span<const uint8_t> message) {
  if (message.size() < kMinMessageSize || message.size() > kMaxMessageSize) {
    return std::nullopt;
  }

  auto left = GetLeft(message);
  auto right = GetRight(message);

  right = GRound(0, left, right);
  left = HRound(0, left, right);
  right = GRound(1, left, right);
  left = HRound(1, left, right);

  left.insert(left.end(), right.begin(), right.end());
  return left;
}

std::optional<std::vector<uint8_t>> RevertF4Jumble(
    base::span<const uint8_t> jumbled_message) {
  if (jumbled_message.size() < kMinMessageSize ||
      jumbled_message.size() > kMaxMessageSize) {
    return std::nullopt;
  }

  auto left = GetLeft(jumbled_message);
  auto right = GetRight(jumbled_message);

  left = HRound(1, left, right);
  right = GRound(1, left, right);
  left = HRound(0, left, right);
  right = GRound(0, left, right);

  left.insert(left.end(), right.begin(), right.end());
  return left;
}

}  // namespace brave_wallet
