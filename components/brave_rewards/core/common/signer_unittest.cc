/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/signer.h"

#include "base/base64.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter='RewardsSignerTest.*'

namespace brave_rewards::internal {

class RewardsSignerTest : public testing::Test {};

TEST_F(RewardsSignerTest, GenerateRecoverySeed) {
  auto seed = Signer::GenerateRecoverySeed();
  EXPECT_FALSE(seed.empty());
  EXPECT_TRUE(Signer::FromRecoverySeed(seed));
}

TEST_F(RewardsSignerTest, SignMessage) {
  auto seed =
      base::Base64Decode("AhqrBVFFNLLfSwKTjPc4KCSf4lb323lmRrcShvJmpGE=");

  auto signer = Signer::FromRecoverySeed(*seed);
  EXPECT_TRUE(signer);

  EXPECT_EQ(base::Base64Encode(signer->public_key()),
            "fmbEEi3h+N1Xzgrsxgbhbp5tXExDGn1jeM02Pvtzs+g=");
  EXPECT_EQ(base::Base64Encode(signer->secret_key()),
            "vasRmhvzIy1J8ij2pSBPvSHPk5DI5l3a08fGSj5JPex+"
            "ZsQSLeH43VfOCuzGBuFunm1cTEMafWN4zTY++3Oz6A==");

  auto signed_message = base::Base64Encode(signer->SignMessage(
      base::byte_span_with_nul_from_cstring("hello world")));

  EXPECT_EQ(signed_message,
            "Yskxukdvz9rLYytvkpsvn2QztIhSbEd9GyUhQ/dX18z/"
            "bUbfOQnFIDybH7DHfGJZxCyjA7AIH0+n9IG/zTalBQ==");
}

}  // namespace brave_rewards::internal
