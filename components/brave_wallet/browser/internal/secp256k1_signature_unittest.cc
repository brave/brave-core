/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/secp256k1_signature.h"

#include <array>

#include "base/containers/to_vector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(Secp256k1Signature, CreateFromPayload) {
  std::array<uint8_t, kSecp256k1CompactSignatureSize> rs_bytes = {
      1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
      17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  EXPECT_FALSE(Secp256k1Signature::CreateFromPayload(rs_bytes, 4));
  EXPECT_FALSE(Secp256k1Signature::CreateFromPayload(rs_bytes, 200));

  EXPECT_TRUE(Secp256k1Signature::CreateFromPayload(rs_bytes, 0));
  EXPECT_TRUE(Secp256k1Signature::CreateFromPayload(rs_bytes, 1));
  EXPECT_TRUE(Secp256k1Signature::CreateFromPayload(rs_bytes, 2));
  EXPECT_TRUE(Secp256k1Signature::CreateFromPayload(rs_bytes, 3));

  auto sig = Secp256k1Signature::CreateFromPayload(rs_bytes, 3);
  EXPECT_EQ(sig->rs_bytes(), base::span(rs_bytes));

  auto bytes = base::ToVector(rs_bytes);
  bytes.push_back(3);
  EXPECT_EQ(sig->bytes(), base::span(bytes));

  EXPECT_EQ(sig->recid(), 3u);
}

}  // namespace brave_wallet
