/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

// npm run test -- brave_unit_tests --filter=RewardsURLLoaderTest.*

namespace brave_rewards::internal {

class RewardsURLLoaderTest : public RewardsEngineTest {};

TEST(RewardsURLLoaderTest, ShouldLogRequestHeader) {
  auto should_log = [](const std::string& header) {
    return URLLoader::ShouldLogRequestHeader(header);
  };

  EXPECT_TRUE(should_log("Content-Type: application/json; charset=UTF-8"));

  EXPECT_TRUE(should_log("Content-type: application/json; charset=UTF-8"));

  EXPECT_TRUE(should_log("digest: a527380a32beee78b46a"));

  EXPECT_TRUE(should_log("Digest: a527380a32beee78b46a"));

  EXPECT_FALSE(should_log("Authorization: Bearer a527380a32beee78b46a"));

  EXPECT_FALSE(should_log("authorization: Bearer a527380a32beee78b46a"));

  EXPECT_FALSE(should_log("Cookie: yummy_cookie=choco;"));

  EXPECT_FALSE(should_log("cookie: yummy_cookie=choco;"));
}

}  // namespace brave_rewards::internal
